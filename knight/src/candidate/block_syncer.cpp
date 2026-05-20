#include "candidate/block_syncer.h"

#include "builder/errors.h"
#include "common/log.h"
#include "common/time.h"
#include "utils/utils.h"

#include <mutex>
#include <utility>

namespace candidate
{

BlockSyncer::BlockSyncer(Config config,
                         net::io_context& io_ctx,
                         std::shared_ptr<IQueue<Event>> queue)
    : m_builder_rest_client(std::make_unique<builder::RestClient>(std::move(config), io_ctx))
    , m_event_queue(std::move(queue))
{}

BlockSyncer::~BlockSyncer()
{
    stop();
}

void BlockSyncer::start()
{
    {
        std::lock_guard lock{m_mutex};
        if (m_running) {
            return;
        }
    }

    set_state(State::STARTING);
    Worker::start();
}

void BlockSyncer::stop()
{
    Worker::stop();
    set_state(State::STOPPED);
}

std::expected<void, Error> BlockSyncer::wait_until_ready(std::chrono::milliseconds timeout)
{
    std::unique_lock lock{m_state_mutex};
    const bool ok = m_state_cv.wait_for(lock, timeout, [this] {
        return m_state == State::READY || m_state == State::FAILED || m_state == State::STOPPED;
    });

    if (!ok) {
        return std::unexpected(Error::TIMEOUT);
    }

    if (m_state != State::READY) {
        return std::unexpected(Error::REQUEST_ERROR);
    }

    return {};
}

BlockSyncer::State BlockSyncer::state() const
{
    std::lock_guard lock{m_state_mutex};
    return m_state;
}

void BlockSyncer::run()
{
    log::info("BlockSyncer", "starting block syncer");

    const auto last_block = fetch_last_block();
    if (!last_block) {
        log::error("BlockSyncer", "failed to initialize last block number");
        set_state(State::FAILED);
        return;
    }

    if (!publish_new_block(*last_block)) {
        log::error("BlockSyncer", "failed to publish initial block event");
        set_state(State::FAILED);
        return;
    }

    m_current_block = *last_block;
    set_state(State::READY);

    log::info("BlockSyncer", "block syncer is ready at block {}", *last_block);
    run_core_loop();
}

void BlockSyncer::run_core_loop()
{
    for (;;) {
        {
            std::unique_lock lock{m_mutex};
            const bool stop_requested = m_cv.wait_for(lock, kPollInterval, [this] {
                return !m_running;
            });
            if (stop_requested) {
                return;
            }
        }

        const auto last_block = fetch_last_block();
        if (!last_block) {
            continue;
        }

        if (m_current_block && *last_block == *m_current_block) {
            continue;
        }

        if (publish_new_block(*last_block)) {
            m_current_block = *last_block;
        }
    }
}

std::optional<uint64_t> BlockSyncer::fetch_last_block() const
{
    const auto head_res = m_builder_rest_client->request_chain_head();
    if (!head_res) {
        log::warn("BlockSyncer", "failed to request chain head: {}", builder::error_to_string(head_res.error()));
        return std::nullopt;
    }

    const auto block_number = parse_last_block(*head_res);
    if (!block_number) {
        log::warn("BlockSyncer", "failed to parse chain head response: {}", *head_res);
        return std::nullopt;
    }

    return block_number;
}

bool BlockSyncer::publish_new_block(uint64_t block_number)
{
    if (!m_event_queue) {
        return false;
    }

    const bool pushed = m_event_queue->try_push(Event{NewBlockEvent{
        .ingress_time = latency_clock::now(),
        .source = "builder",
        .block_number = block_number,
    }});

    if (!pushed) {
        log::warn("BlockSyncer", "drop new block event: block_number={}", block_number);
        return false;
    }

    log::info("BlockSyncer", "new block event published: block_number={}", block_number);
    return true;
}

void BlockSyncer::set_state(State state)
{
    {
        std::lock_guard lock{m_state_mutex};
        m_state = state;
    }

    m_state_cv.notify_all();
}

std::optional<uint64_t> BlockSyncer::parse_last_block(std::string_view body)
{
    const auto json = parse_to_json_object(body);
    if (!json) {
        return std::nullopt;
    }

    const auto block_number_raw = json_string(*json, "blockNumber");
    if (!block_number_raw) {
        return std::nullopt;
    }

    return parse_hex_quantity(*block_number_raw);
}

} // namespace candidate
