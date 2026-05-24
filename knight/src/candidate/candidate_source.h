#pragma once

#include "errors.h"
#include "candidate/block_syncer.h"
#include "candidate/candidate_filter.h"
#include "candidate/local_mempool.h"
#include "builder/pending_feed.h"
#include "builder/rest_client.h"
#include "common/config.h"
#include "candidate/event.h"
#include "common/queue.h"
#include "common/worker.h"

#include <boost/asio.hpp>

#include <cstdint>
#include <expected>
#include <memory>

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
    [[nodiscard]] std::expected<Candidate, Error> wait_pop_next_candidate();

private:
    void run() override;
    void stop_inputs();
    std::expected<void, Error> run_core_loop();
    void handle_new_block_event(const NewBlockEvent& event);
    void handle_pending_tx_event(PendingTransactionEvent event);
    void publish_new_block(uint64_t block_number);
    void publish_pending_transaction(builder::PendingTransaction transaction);

private:
    Config m_config;
    net::io_context& m_io_ctx;

    std::shared_ptr<IQueue<Event>> m_pending_queue;
    std::unique_ptr<builder::RestClient> m_builder_rest_client;
    std::unique_ptr<builder::PendingFeed> m_pending_feed;
    std::unique_ptr<BlockSyncer> m_block_syncer;
    CandidateFilter m_candidate_filter;
    LocalMempool m_local_mempool;
};

} // namespace candidate
