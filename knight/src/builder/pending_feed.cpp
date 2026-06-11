#include "builder/pending_feed.h"

#include "common/log.h"
#include "utils/utils.h"

#include <optional>
#include <string_view>
#include <utility>

namespace builder
{
namespace
{

std::optional<PendingTransaction> parse_pending_tx_event(std::string_view payload)
{
    const auto json = parse_to_json_object(payload);
    if (!json) {
        return std::nullopt;
    }

    const auto event_type = json_string(*json, "type");
    const auto* record = json->if_contains("record");
    if (!event_type || *event_type != "pending_transaction" || record == nullptr) {
        return std::nullopt;
    }

    return PendingTransaction::from_json(*record);
}

} // namespace

PendingFeed::PendingFeed(Config config,
                         net::io_context& io_ctx,
                         pending_transaction_handler_t on_pending_transaction)
    : m_config(std::move(config)),
      m_io_ctx(io_ctx),
      m_on_pending_transaction(std::move(on_pending_transaction))
{
    const auto& endpoint = m_config.builder_ws_endpoint;
    log::info("PendingFeed", "mempool stream endpoint: {}", m_config.builder_ws_url);

    m_ws_source = std::make_unique<network::WsSource>(
        m_io_ctx,
        endpoint.use_tls,
        m_config.tls_verify_peer,
        endpoint.host,
        endpoint.port,
        endpoint.target,
        [this](std::string payload) {
            on_ws_message(std::move(payload));
        },
        [this](beast::error_code ec, std::string_view where) {
            on_ws_error(ec, where);
        },
        [this](network::WsSource::State state) {
            on_ws_state(state);
        });
}

void PendingFeed::open()
{
    if (m_state == State::OPEN) {
        return;
    }

    log::info("PendingFeed", "opening mempool stream...");
    m_ws_source->start();
}

void PendingFeed::close()
{
    if (m_state == State::CLOSED) {
        return;
    }

    log::info("PendingFeed", "closing mempool stream...");
    m_ws_source->stop();
}

void PendingFeed::reopen()
{
    log::info("PendingFeed", "reopen requested");
    m_ws_source->restart();
}

void PendingFeed::on_ws_message(std::string payload)
{
    auto candidate = parse_pending_tx_event(payload);
    if (!candidate) {
        log::warn("PendingFeed", "failed to parse pending tx payload: {}", payload);
        return;
    }

    if (!m_on_pending_transaction) {
        log::warn("PendingFeed",
                  "drop pending tx because no handler is configured: seq_num={}",
                  candidate->seq_num);
        return;
    }

    m_on_pending_transaction(std::move(*candidate));
}

void PendingFeed::on_ws_state(network::WsSource::State state)
{
    log::info("PendingFeed", "websocket state: {}", ws_source_state_to_string(state));
    switch (state) {
    case network::WsSource::State::STOPPED:
        set_state(State::CLOSED);
        break;
    case network::WsSource::State::STARTING:
        break;
    case network::WsSource::State::RUNNING:
        set_state(State::OPEN);
        break;
    case network::WsSource::State::STOPPING:
        break;
    case network::WsSource::State::FAILED:
        set_state(State::FAILED);
        break;
    }
}

void PendingFeed::on_ws_error(beast::error_code ec, std::string_view where)
{
    log::error("PendingFeed", "websocket error: {} {}", where, ec.message());
    set_state(State::FAILED);
}

void PendingFeed::set_state(State state)
{
    {
        std::lock_guard lock{m_state_mutex};
        m_state = state;
    }

    m_state_cv.notify_all();
}

std::expected<void, Error> PendingFeed::wait_until_ready(std::chrono::milliseconds timeout)
{
    std::unique_lock lock{m_state_mutex};
    const bool ok = m_state_cv.wait_for(lock, timeout, [this] {
        return m_state == State::OPEN || m_state == State::FAILED;
    });

    if (!ok) {
        return std::unexpected(Error::TIMEOUT);
    }

    if (m_state == State::FAILED) {
        return std::unexpected(Error::REQUEST_ERROR);
    }

    return {};
}

} // namespace builder
