#pragma once

#include "common/event.h"
#include "common/queue.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace net = boost::asio;
namespace ssl = net::ssl;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = net::ip::tcp;

namespace network
{

class WsSource
{
public:
    enum class State
    {
        STOPPED,
        STARTING,
        RUNNING,
        STOPPING,
        FAILED
    };

    using error_handler_t = std::function<void(beast::error_code, std::string_view)>;
    using state_handler_t = std::function<void(State)>;
    using drop_handler_t = std::function<void()>;

    WsSource(net::io_context& io_ctx,
             std::shared_ptr<IQueue<Event>> queue,
             bool use_tls,
             bool verify_tls_peer,
             std::string host,
             std::string port,
             std::string target,
             error_handler_t on_error = {},
             state_handler_t on_state = {},
             drop_handler_t on_drop = {});

    void start();
    void stop();
    void restart();

    [[nodiscard]] State state() const noexcept;

private:
    using PlainWsStream = websocket::stream<tcp::socket>;
    using TlsWsStream = websocket::stream<beast::ssl_stream<tcp::socket>>;

    template <typename Handler>
    void with_stream(Handler&& handler)
    {
        if (m_use_tls) {
            handler(*m_tls_ws);
            return;
        }

        handler(*m_plain_ws);
    }

    void do_resolve();
    void do_read();
    void do_close();
    void do_ws_handshake();

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void on_connect(beast::error_code ec);
    void on_tls_handshake(beast::error_code ec);
    void on_ws_handshake(beast::error_code ec);
    void on_read(beast::error_code ec, size_t bytes);
    void on_close(beast::error_code ec);

    void publish_message(std::string payload);
    void publish_error(beast::error_code ec, std::string_view where);
    void publish_state(State state);
    void fail(beast::error_code ec, std::string_view where);
    void schedule_reconnect(std::string_view reason);
    void reset_stream();

private:
    net::io_context& m_io_ctx;
    tcp::resolver m_resolver;
    ssl::context m_ssl_context;
    std::unique_ptr<PlainWsStream> m_plain_ws;
    std::unique_ptr<TlsWsStream> m_tls_ws;
    beast::flat_buffer m_buffer;
    net::steady_timer m_reconnect_timer;

    std::shared_ptr<IQueue<Event>> m_queue;

    bool m_use_tls{false};
    bool m_verify_tls_peer{true};
    std::string m_host;
    std::string m_port;
    std::string m_target;

    State m_state{State::STOPPED};
    bool m_restart_requested{false};
    size_t m_reconnect_attempts{0};

    error_handler_t m_on_error;
    state_handler_t m_on_state;
    drop_handler_t m_on_drop;

    static constexpr size_t kMaxReconnectAttempts{10};
    static constexpr std::chrono::milliseconds kReconnectBaseDelay{200};
    static constexpr std::chrono::milliseconds kReconnectMaxDelay{5000};
};

std::string ws_source_state_to_string(WsSource::State state);

} // namespace network
