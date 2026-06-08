#pragma once

#include "errors.h"
#include "candidate/block_syncer.h"
#include "candidate/candidate_filter.h"
#include "candidate/state.h"
#include "builder/pending_feed.h"
#include "builder/rest_client.h"
#include "common/config.h"
#include "candidate/event.h"
#include "common/queue.h"
#include "common/worker.h"

#include <boost/asio.hpp>

#include <condition_variable>
#include <cstdint>
#include <expected>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace net = boost::asio;

namespace candidate
{

class CandidateSource final : public Worker
{
public:
    CandidateSource(Config config, net::io_context& io_ctx);
    ~CandidateSource() override;

    CandidateSource(const CandidateSource&) = delete;
    CandidateSource& operator=(const CandidateSource&) = delete;

    void stop();
    [[nodiscard]] std::expected<StateSnapshot, Error> wait_pop_state();

private:
    void run() override;
    void stop_inputs();
    void close_state_waiters();
    std::expected<void, Error> run_core_loop();
    void handle_new_block_event(const NewBlockEvent& event);
    void handle_pending_tx_event(PendingTransactionEvent event);
    void publish_new_block(uint64_t block_number);
    void publish_pending_transaction(builder::PendingTransaction transaction);
    std::optional<std::vector<PoolSnapshot>> request_pool_snapshots(uint64_t block_number) const;
    bool mark_state_invalid(uint64_t block_number);
    bool apply_state_snapshot(CandidateSnapshot snapshot, std::vector<PoolSnapshot> pools);
    bool apply_pending_candidate(Candidate candidate);

private:
    Config m_config;
    net::io_context& m_io_ctx;

    std::shared_ptr<IQueue<Event>> m_pending_queue;
    std::unique_ptr<builder::RestClient> m_builder_rest_client;
    std::unique_ptr<builder::PendingFeed> m_pending_feed;
    std::unique_ptr<BlockSyncer> m_block_syncer;
    std::vector<evm::Pool> m_pools;
    CandidateFilter m_candidate_filter;
    State m_state;

    mutable std::mutex m_state_wait_mutex;
    std::condition_variable m_state_cv;
    bool m_closed{false};
    bool m_invalid_snapshot_pending{false};
};

} // namespace candidate
