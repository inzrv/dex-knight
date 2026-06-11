#include "candidate/candidate_source.h"

#include "builder/errors.h"
#include "common/log.h"
#include "evm/contracts/sandbox_dex.h"
#include "utils/utils.h"

#include <boost/json.hpp>

#include <chrono>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace candidate
{
namespace
{

std::vector<evm::Pool> make_pools(const std::vector<PoolConfig>& pools)
{
    std::vector<evm::Pool> result;
    result.reserve(pools.size());

    for (const auto& pool : pools) {
        result.emplace_back(pool.address, pool.token_a, pool.token_b);
    }

    return result;
}

} // namespace

CandidateSource::CandidateSource(Config config, net::io_context& io_ctx)
    : m_config(std::move(config)),
      m_io_ctx(io_ctx),
      m_pending_queue(std::make_shared<Queue<Event, 10'000>>()),
      m_builder_rest_client(std::make_unique<builder::RestClient>(m_config, m_io_ctx)),
      m_pending_feed(std::make_unique<builder::PendingFeed>(
          m_config,
          m_io_ctx,
          [this](builder::PendingTransaction transaction) {
              publish_pending_transaction(std::move(transaction));
          })),
      m_block_syncer(std::make_unique<BlockSyncer>(m_config,
                                                   m_io_ctx,
                                                   [this](uint64_t block_number) {
                                                       publish_new_block(block_number);
                                                   })),
      m_pools(make_pools(m_config.pools)),
      m_candidate_filter(m_pools)
{}

CandidateSource::~CandidateSource()
{
    stop();
}

void CandidateSource::stop()
{
    stop_inputs();
    m_pending_queue->close();
    close_state_waiters();

    Worker::stop();
}

std::expected<StateSnapshot, Error> CandidateSource::wait_pop_state()
{
    std::unique_lock lock{m_state_wait_mutex};
    m_state_cv.wait(lock, [this] {
        return m_closed || m_invalid_snapshot_pending || m_state.size() > 0;
    });

    if (m_closed) {
        return std::unexpected(Error::CLOSED);
    }

    if (m_invalid_snapshot_pending) {
        m_invalid_snapshot_pending = false;
        return m_state.snapshot();
    }

    auto snapshot = m_state.pop_next_snapshot();
    if (!snapshot) {
        return std::unexpected(Error::CLOSED);
    }

    return std::move(*snapshot);
}

void CandidateSource::stop_inputs()
{
    m_pending_feed->close();
    m_block_syncer->stop();
}

void CandidateSource::run()
{
    log::info("CandidateSource", "starting block syncer");
    m_block_syncer->start();

    const auto block_sync_res = m_block_syncer->wait_until_ready(std::chrono::seconds{10});
    if (!block_sync_res) {
        log::error("CandidateSource",
                   "failed to start block syncer: {}",
                   candidate::error_to_string(block_sync_res.error()));
        stop_inputs();
        close_state_waiters();
        return;
    }

    log::info("CandidateSource", "block syncer ready");

    log::info("CandidateSource", "starting builder pending feed");
    m_pending_feed->open();

    const auto wait_res = m_pending_feed->wait_until_ready(std::chrono::seconds{10});
    if (!wait_res) {
        log::error("CandidateSource",
                   "failed to open builder pending feed: {}",
                   builder::error_to_string(wait_res.error()));
        stop_inputs();
        close_state_waiters();
        return;
    }

    log::info("CandidateSource", "builder pending feed ready");

    const auto loop_res = run_core_loop();
    if (!loop_res) {
        log::error("CandidateSource", "core loop error: {}", error_to_string(loop_res.error()));
        close_state_waiters();
    }
}

void CandidateSource::close_state_waiters()
{
    {
        std::lock_guard lock{m_state_wait_mutex};
        m_closed = true;
    }

    m_state_cv.notify_all();
}

std::expected<void, Error> CandidateSource::run_core_loop()
{
    for (;;) {
        auto event = m_pending_queue->wait_pop();
        if (!event) {
            log::info("CandidateSource", "event queue closed");
            return {};
        }

        if (auto* pending_tx = std::get_if<PendingTransactionEvent>(&*event)) {
            handle_pending_tx_event(std::move(*pending_tx));
            continue;
        }

        if (const auto* new_block = std::get_if<NewBlockEvent>(&*event)) {
            handle_new_block_event(*new_block);
            continue;
        }
    }
}

void CandidateSource::handle_new_block_event(const NewBlockEvent& event)
{
    log::info("CandidateSource",
              "new block event: source={} block_number={}",
              event.source,
              event.block_number);

    auto pool_snapshots = request_pool_snapshots(event.block_number);
    if (!pool_snapshots) {
        log::warn("CandidateSource",
                  "failed to request pool state snapshot: block_number={}",
                  event.block_number);
        mark_state_invalid(event.block_number);
        return;
    }

    const auto snapshot_res = m_builder_rest_client->request_snapshot();
    if (!snapshot_res) {
        log::warn("CandidateSource",
                  "failed to request pending snapshot: {}",
                  builder::error_to_string(snapshot_res.error()));
        mark_state_invalid(event.block_number);
        return;
    }

    const auto snapshot_json = parse_to_json_object(*snapshot_res);
    if (!snapshot_json) {
        log::warn("CandidateSource", "failed to parse pending snapshot json");
        mark_state_invalid(event.block_number);
        return;
    }

    auto snapshot = builder::PendingSnapshot::from_json(*snapshot_json, event.block_number);
    if (!snapshot) {
        log::warn("CandidateSource", "failed to parse pending snapshot");
        mark_state_invalid(event.block_number);
        return;
    }

    auto candidate_snapshot = m_candidate_filter.filter_snapshot(std::move(*snapshot));
    const auto snapshot_seq = candidate_snapshot.snapshot_seq;
    const auto candidate_count = candidate_snapshot.candidates.size();
    const auto pool_count = pool_snapshots->size();
    const bool applied =
        apply_state_snapshot(std::move(candidate_snapshot), std::move(*pool_snapshots));
    if (!applied) {
        const auto current_block = m_state.block_number();
        const auto current_block_text =
            current_block ? std::to_string(*current_block) : std::string{"none"};
        log::debug("CandidateSource",
                   "ignored stale pending snapshot: block_number={} snapshot_seq={} "
                   "current_block_number={} current_snapshot_seq={}",
                   event.block_number,
                   snapshot_seq,
                   current_block_text,
                   m_state.snapshot_seq());
        return;
    }

    log::info("CandidateSource",
              "state snapshot applied: block_number={} snapshot_seq={} pools={} candidates={}",
              event.block_number,
              snapshot_seq,
              pool_count,
              candidate_count);
}

void CandidateSource::handle_pending_tx_event(PendingTransactionEvent event)
{
    const auto seq_num = event.tx.seq_num;
    auto candidate = m_candidate_filter.filter(std::move(event.tx));
    if (!candidate) {
        log::debug(
            "CandidateSource", "ignored pending tx event outside filter: seq_num={}", seq_num);
        return;
    }

    const bool accepted = apply_pending_candidate(std::move(*candidate));
    if (!accepted) {
        log::debug("CandidateSource",
                   "ignored pending tx event covered by snapshot or invalid state: seq_num={} "
                   "snapshot_seq={} valid={}",
                   seq_num,
                   m_state.snapshot_seq(),
                   m_state.valid());
        return;
    }

    log::info("CandidateSource",
              "pending tx candidate accepted: seq_num={} candidates={}",
              seq_num,
              m_state.size());
}

void CandidateSource::publish_new_block(uint64_t block_number)
{
    const bool pushed = m_pending_queue->try_push(Event{NewBlockEvent{
        .ingress_time = latency_clock::now(),
        .source = "builder",
        .block_number = block_number,
    }});

    if (!pushed) {
        log::warn("CandidateSource", "drop new block event: block_number={}", block_number);
    }
}

void CandidateSource::publish_pending_transaction(builder::PendingTransaction transaction)
{
    const auto seq_num = transaction.seq_num;
    const bool pushed = m_pending_queue->try_push(Event{PendingTransactionEvent{
        .ingress_time = latency_clock::now(),
        .source = m_config.builder_ws_endpoint.host,
        .tx = std::move(transaction),
    }});

    if (!pushed) {
        log::warn("CandidateSource", "drop pending tx event: seq_num={}", seq_num);
    }
}

std::optional<std::vector<PoolSnapshot>> CandidateSource::request_pool_snapshots(
    uint64_t block_number) const
{
    std::vector<PoolSnapshot> result;
    result.reserve(m_pools.size());

    for (const auto& pool : m_pools) {
        boost::json::object payload;
        payload["to"] = hex_data(pool.address);
        payload["data"] = hex_data(evm::SandboxDex::GetReserves::selector);
        payload["block"] = hex_quantity(block_number);

        const auto raw_response = m_builder_rest_client->request_chain_call(payload);
        if (!raw_response) {
            log::warn("CandidateSource",
                      "failed to request pool reserves: pool={} block_number={} error={}",
                      hex_data(pool.address),
                      block_number,
                      builder::error_to_string(raw_response.error()));
            return std::nullopt;
        }

        const auto response_json = parse_to_json(*raw_response);
        if (!response_json) {
            log::warn("CandidateSource",
                      "failed to parse pool reserves response json: pool={} block_number={}",
                      hex_data(pool.address),
                      block_number);
            return std::nullopt;
        }

        auto pool_snapshot = PoolSnapshot::from_json(pool, *response_json);
        if (!pool_snapshot) {
            log::warn("CandidateSource",
                      "failed to parse pool reserves response: pool={} block_number={}",
                      hex_data(pool.address),
                      block_number);
            return std::nullopt;
        }

        result.push_back(std::move(*pool_snapshot));
    }

    return result;
}

bool CandidateSource::mark_state_invalid(uint64_t block_number)
{
    bool marked = false;

    {
        std::lock_guard lock{m_state_wait_mutex};
        if (m_closed) {
            return false;
        }

        marked = m_state.mark_invalid(block_number);
        if (marked) {
            m_invalid_snapshot_pending = true;
        }
    }

    if (marked) {
        m_state_cv.notify_one();
    }
    return marked;
}

bool CandidateSource::apply_state_snapshot(CandidateSnapshot snapshot,
                                           std::vector<PoolSnapshot> pools)
{
    bool applied = false;
    bool should_notify = false;

    {
        std::lock_guard lock{m_state_wait_mutex};
        if (m_closed) {
            return false;
        }

        applied = m_state.apply_snapshot(std::move(snapshot), std::move(pools));
        if (applied) {
            m_invalid_snapshot_pending = false;
            should_notify = m_state.size() > 0;
        }
    }

    if (should_notify) {
        m_state_cv.notify_one();
    }
    return applied;
}

bool CandidateSource::apply_pending_candidate(Candidate candidate)
{
    bool accepted = false;

    {
        std::lock_guard lock{m_state_wait_mutex};
        if (m_closed) {
            return false;
        }

        accepted = m_state.apply_pending_tx(std::move(candidate));
    }

    if (accepted) {
        m_state_cv.notify_one();
    }
    return accepted;
}

} // namespace candidate
