#pragma once

#include "errors.h"
#include "builder/pending_feed.h"
#include "builder/rest_client.h"
#include "common/config.h"
#include "common/queue.h"
#include "common/worker.h"

#include <boost/asio.hpp>

#include <expected>
#include <memory>
#include <mutex>

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

private:
    void run() override;
    std::expected<void, Error> run_core_loop();

private:
    Config m_config;
    net::io_context& m_io_ctx;

    std::shared_ptr<IQueue> m_pending_queue;
    std::unique_ptr<builder::RestClient> m_builder_rest_client;
    std::unique_ptr<builder::PendingFeed> m_pending_feed;
};

} // namespace candidate
