#include "runtime/runtime_factory.h"

#include "common/log.h"

#include <utility>

namespace runtime
{

RuntimeFactory::RuntimeFactory(Config config)
    : m_config(std::move(config))
{}

RuntimeComponents RuntimeFactory::create(boost::asio::io_context& io_ctx)
{
    log::info("RuntimeFactory", "creating components...");

    RuntimeComponents components;
    components.candidate_source = std::make_unique<candidate::CandidateSource>(m_config, io_ctx);

    log::info("RuntimeFactory", "created all components");
    return components;
}

} // namespace runtime
