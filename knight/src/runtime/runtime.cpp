#include "runtime/runtime.h"

#include "common/log.h"

#include <chrono>
#include <stdexcept>
#include <thread>
#include <utility>

namespace runtime
{

Runtime::Runtime(RuntimeFactory& factory)
    : m_io_ctx()
    , m_work_guard(net::make_work_guard(m_io_ctx))
{
    auto components = factory.create(m_io_ctx);
    m_candidate_source = std::move(components.candidate_source);

    if (!m_candidate_source) {
        throw std::invalid_argument("runtime factory returned incomplete components");
    }

    log::info("Runtime", "Runtime initialized with all components");
}

Runtime::~Runtime()
{
    stop();
}

void Runtime::run()
{
    log::info("Runtime", "starting...");
    m_running = true;

    m_io_thread = std::thread([this]() {
        m_io_ctx.run();
    });

    m_candidate_source->start();
    run_core_loop();
}

void Runtime::stop()
{
    if (!m_running.exchange(false)) {
        return;
    }

    log::info("Runtime", "stopping...");
    m_candidate_source->stop();
    m_work_guard.reset();
    m_io_ctx.stop();

    if (m_io_thread.joinable()) {
        m_io_thread.join();
    }
}

void Runtime::run_core_loop()
{
    while (m_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds{200});
    }

    log::info("Runtime", "core loop stopped");
}

} // namespace runtime
