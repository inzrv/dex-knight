#include "ws_source.h"

#include "common/log.h"

#include <boost/asio/connect.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include <utility>

namespace network
{

std::string ws_source_state_to_string(WsSource::State state)
{
    switch (state) {
        case WsSource::State::STOPPED: return "STOPPED";
        case WsSource::State::STARTING: return "STARTING";
        case WsSource::State::RUNNING: return "RUNNING";
        case WsSource::State::STOPPING: return "STOPPING";
        case WsSource::State::FAILED: return "FAILED";
    }
    return "UNKNOWN";
}

WsSource::WsSource(net::io_context& io_ctx,
                   std::shared_ptr<IQueue<InputEnvelope>> queue,
                   bool use_tls,
                   bool verify_tls_peer,
                   std::string host,
                   std::string port,
                   std::string target,
                   error_handler_t on_error,
                   state_handler_t on_state,
                   drop_handler_t on_drop)
    : m_io_ctx(io_ctx)
    , m_resolver(net::make_strand(io_ctx))
    , m_ssl_context(ssl::context::tls_client)
    , m_reconnect_timer(io_ctx)
    , m_queue(std::move(queue))
    , m_use_tls(use_tls)
    , m_verify_tls_peer(verify_tls_peer)
    , m_host(std::move(host))
    , m_port(std::move(port))
    , m_target(std::move(target))
    , m_on_error(std::move(on_error))
    , m_on_state(std::move(on_state))
    , m_on_drop(std::move(on_drop))
{
    if (m_use_tls) {
        beast::error_code ec;
        m_ssl_context.set_default_verify_paths(ec);
        if (ec) {
            log::warn("WsSource", "could not load default TLS verify paths: {}", ec.message());
        }

        m_ssl_context.set_verify_mode(m_verify_tls_peer ? ssl::verify_peer : ssl::verify_none);
    }

    reset_stream();
}

void WsSource::start()
{
    m_reconnect_timer.cancel();
    if (m_state == State::STARTING || m_state == State::RUNNING) {
        return;
    }

    log::info("WsSource", "starting... scheme={} host={} target={}", m_use_tls ? "wss" : "ws", m_host, m_target);
    m_restart_requested = false;
    reset_stream();
    publish_state(State::STARTING);
    do_resolve();
}

void WsSource::stop()
{
    log::info("WsSource", "stopping...");
    m_restart_requested = false;
    m_reconnect_timer.cancel();

    if (m_state == State::STOPPED || m_state == State::STOPPING) {
        return;
    }

    publish_state(State::STOPPING);
    do_close();
}

void WsSource::restart()
{
    log::info("WsSource", "restart requested");
    m_restart_requested = true;

    if (m_state == State::STOPPED || m_state == State::FAILED) {
        reset_stream();
        start();
        return;
    }

    if (m_state == State::STOPPING) {
        return;
    }

    publish_state(State::STOPPING);
    do_close();
}

WsSource::State WsSource::state() const noexcept
{
    return m_state;
}

void WsSource::reset_stream()
{
    m_buffer.consume(m_buffer.size());
    m_resolver.cancel();

    if (m_plain_ws) {
        beast::error_code ec;
        beast::get_lowest_layer(*m_plain_ws).close(ec);
    }

    if (m_tls_ws) {
        beast::error_code ec;
        beast::get_lowest_layer(*m_tls_ws).close(ec);
    }

    m_plain_ws.reset();
    m_tls_ws.reset();

    if (m_use_tls) {
        m_tls_ws = std::make_unique<TlsWsStream>(net::make_strand(m_io_ctx), m_ssl_context);
    } else {
        m_plain_ws = std::make_unique<PlainWsStream>(net::make_strand(m_io_ctx));
    }
}

void WsSource::do_resolve()
{
    m_resolver.async_resolve(
        m_host,
        m_port,
        beast::bind_front_handler(&WsSource::on_resolve, this));
}

void WsSource::do_read()
{
    with_stream([this](auto& ws) {
        ws.async_read(
            m_buffer,
            beast::bind_front_handler(&WsSource::on_read, this));
    });
}

void WsSource::do_close()
{
    bool is_open = false;
    with_stream([&is_open](auto& ws) {
        is_open = beast::get_lowest_layer(ws).is_open();
    });

    if (!is_open) {
        on_close({});
        return;
    }

    with_stream([this](auto& ws) {
        ws.async_close(
            websocket::close_code::normal,
            beast::bind_front_handler(&WsSource::on_close, this));
    });
}

void WsSource::do_ws_handshake()
{
    with_stream([this](auto& ws) {
        ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
        ws.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req) {
                req.set(beast::http::field::user_agent, "knight-ws-source");
            }));

        const std::string host_header = m_host + ":" + m_port;
        ws.async_handshake(
            host_header,
            m_target,
            beast::bind_front_handler(&WsSource::on_ws_handshake, this));
    });
}

void WsSource::on_resolve(beast::error_code ec, tcp::resolver::results_type results)
{
    if (ec) {
        log::error("WsSource", "resolve failed: {}", ec.message());
        fail(ec, "resolve");
        return;
    }

    log::debug("WsSource", "resolved host");
    if (m_use_tls) {
        net::async_connect(
            beast::get_lowest_layer(*m_tls_ws),
            results,
            [this](beast::error_code ec, const tcp::endpoint&) {
                on_connect(ec);
            });
        return;
    }

    net::async_connect(
        beast::get_lowest_layer(*m_plain_ws),
        results,
        [this](beast::error_code ec, const tcp::endpoint&) {
            on_connect(ec);
        });
}

void WsSource::on_connect(beast::error_code ec)
{
    if (ec) {
        log::error("WsSource", "connect failed: {}", ec.message());
        fail(ec, "connect");
        return;
    }

    log::debug("WsSource", "connected to host");

    if (!m_use_tls) {
        do_ws_handshake();
        return;
    }

    auto* native_tls = m_tls_ws->next_layer().native_handle();
    if (!SSL_set_tlsext_host_name(native_tls, m_host.c_str())) {
        beast::error_code sni_ec(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category());
        log::error("WsSource", "TLS SNI setup failed: {}", sni_ec.message());
        fail(sni_ec, "tls_sni");
        return;
    }

    if (m_verify_tls_peer && !SSL_set1_host(native_tls, m_host.c_str())) {
        beast::error_code host_ec(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category());
        log::error("WsSource", "TLS hostname verification setup failed: {}", host_ec.message());
        fail(host_ec, "tls_verify_host");
        return;
    }

    m_tls_ws->next_layer().async_handshake(
        ssl::stream_base::client,
        beast::bind_front_handler(&WsSource::on_tls_handshake, this));
}

void WsSource::on_tls_handshake(beast::error_code ec)
{
    if (ec) {
        log::error("WsSource", "TLS handshake failed: {}", ec.message());
        fail(ec, "tls_handshake");
        return;
    }

    log::debug("WsSource", "TLS handshake succeeded");
    do_ws_handshake();
}

void WsSource::on_ws_handshake(beast::error_code ec)
{
    if (ec) {
        log::error("WsSource", "websocket handshake failed: {}", ec.message());
        fail(ec, "ws_handshake");
        return;
    }

    log::info("WsSource", "websocket handshake succeeded");
    m_reconnect_attempts = 0;
    publish_state(State::RUNNING);
    do_read();
}

void WsSource::on_read(beast::error_code ec, size_t /*bytes*/)
{
    if (ec) {
        log::error("WsSource", "read failed: {}", ec.message());
        fail(ec, "read");
        return;
    }

    publish_message(beast::buffers_to_string(m_buffer.data()));
    m_buffer.consume(m_buffer.size());

    if (m_state == State::RUNNING) {
        do_read();
    }
}

void WsSource::on_close(beast::error_code ec)
{
    m_buffer.consume(m_buffer.size());

    const bool close_failed = ec && ec != websocket::error::closed;

    if (close_failed) {
        log::warn("WsSource", "close failed: {}", ec.message());
        publish_error(ec, "close");
        publish_state(State::FAILED);
        schedule_reconnect("close_failed");
    } else {
        log::info("WsSource", "closed cleanly");
        publish_state(State::STOPPED);
    }

    reset_stream();

    if (m_restart_requested) {
        m_restart_requested = false;
        start();
    }
}

void WsSource::publish_message(std::string payload)
{
    if (!m_queue) {
        return;
    }

    const bool pushed = m_queue->try_push(InputEnvelope{
        .ingress_time = latency_clock::now(),
        .source = m_host,
        .payload = std::move(payload)
    });

    if (!pushed && m_on_drop) {
        m_on_drop();
    }
}

void WsSource::publish_error(beast::error_code ec, std::string_view where)
{
    if (m_on_error) {
        m_on_error(ec, where);
    }
}

void WsSource::publish_state(State state)
{
    m_state = state;

    if (m_on_state) {
        m_on_state(state);
    }
}

void WsSource::fail(beast::error_code ec, std::string_view where)
{
    log::error("WsSource", " failure at {}: {}", where, ec.message());
    publish_error(ec, where);
    publish_state(State::FAILED);
    reset_stream();
    schedule_reconnect(where);
}

void WsSource::schedule_reconnect(std::string_view reason)
{
    if (m_state == State::STOPPING || m_state == State::STOPPED) {
        return;
    }

    if (m_reconnect_attempts >= kMaxReconnectAttempts) {
        log::error("WsSource",
                   "auto-reconnect disabled after {} attempts (last reason: {})",
                   m_reconnect_attempts,
                   reason);
        return;
    }

    const auto attempt = m_reconnect_attempts++;
    const auto multiplier = static_cast<int64_t>(1ull << std::min<size_t>(attempt, 5));
    const auto exp_delay = std::chrono::milliseconds{kReconnectBaseDelay.count() * multiplier};
    const auto delay = std::min(exp_delay, kReconnectMaxDelay);

    log::warn("WsSource",
              "scheduling auto-reconnect attempt {}/{} in {} ms (reason: {})",
              m_reconnect_attempts,
              kMaxReconnectAttempts,
              delay.count(),
              reason);

    m_reconnect_timer.expires_after(delay);
    m_reconnect_timer.async_wait([this](beast::error_code ec) {
        if (ec) {
            return;
        }

        if (m_state == State::STOPPING || m_state == State::RUNNING) {
            return;
        }

        start();
    });
}

} // namespace network
