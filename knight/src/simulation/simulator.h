#pragma once

#include "builder/bundle.h"
#include "builder/bundle_result.h"
#include "builder/bundle_sim_result.h"
#include "builder/errors.h"
#include "builder/rest_client.h"
#include "common/config.h"

#include <boost/asio/io_context.hpp>

#include <expected>

namespace simulation
{

class Simulator final
{
public:
    Simulator(Config config, boost::asio::io_context& io_ctx);

    std::expected<builder::BundleSimulationResult, builder::Error> simulate(
        const builder::Bundle& bundle) const;
    std::expected<builder::BundleResult, builder::Error> submit(
        const builder::Bundle& bundle) const;

private:
    builder::RestClient m_builder_rest_client;
};

} // namespace simulation
