#pragma once

#include "runtime/runtime_factory.h"

#include <boost/asio.hpp>

#include <atomic>
#include <memory>
#include <thread>

namespace net = boost::asio;

namespace runtime
{

class Runtime
{
public:
    explicit Runtime(RuntimeFactory& factory);
    ~Runtime();

    void run();
    void stop();

private:
    void run_core_loop();

private:
    net::io_context m_io_ctx;
    net::executor_work_guard<net::io_context::executor_type> m_work_guard;
    std::thread m_io_thread;
    std::atomic<bool> m_running{false};

    std::unique_ptr<candidate::CandidateSource> m_candidate_source;
};

} // namespace runtime
