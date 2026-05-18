#include "candidate/candidate_source.h"

#include "builder/errors.h"
#include "common/log.h"

#include <chrono>
#include <type_traits>
#include <utility>
#include <variant>

namespace candidate
{

CandidateSource::CandidateSource(Config config, net::io_context& io_ctx)
    : m_config(std::move(config))
    , m_io_ctx(io_ctx)
    , m_pending_queue(std::make_shared<Queue<Event, 10'000>>())
    , m_builder_rest_client(std::make_unique<builder::RestClient>(m_config, m_io_ctx))
    , m_pending_feed(std::make_unique<builder::PendingFeed>(
        m_config,
        m_io_ctx,
        m_pending_queue))
{}

CandidateSource::~CandidateSource()
{
    stop();
}

void CandidateSource::stop()
{
    m_pending_feed->close();
    m_pending_queue->close();

    Worker::stop();
}

void CandidateSource::run()
{
    log::info("CandidateSource", "starting builder pending feed");
    m_pending_feed->open();

    const auto wait_res = m_pending_feed->wait_until_ready(std::chrono::seconds{10});
    if (!wait_res) {
        log::error("CandidateSource", "failed to open builder pending feed: {}", builder::error_to_string(wait_res.error()));
        return;
    }

    log::info("CandidateSource", "builder pending feed ready");

    const auto loop_res = run_core_loop();
    if (!loop_res) {
        log::error("CandidateSource", "core loop error: {}", error_to_string(loop_res.error()));
    }
}

std::expected<void, Error> CandidateSource::run_core_loop()
{
    for (;;) {
        auto event = m_pending_queue->wait_pop();
        if (!event) {
            log::info("CandidateSource", "event queue closed");
            return {};
        }

        std::visit([](const auto& item) {
            using EventType = std::decay_t<decltype(item)>;

            if constexpr (std::is_same_v<EventType, PendingTxEvent>) {
                log::info("CandidateSource", "pending tx payload: {}", item.payload);
            } else if constexpr (std::is_same_v<EventType, NewBlockEvent>) {
                log::info("CandidateSource", "new block event: source={} block_number={}", item.source, item.block_number);
            }
        }, *event);
    }
}

} // namespace candidate
