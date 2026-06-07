#pragma once

#include "builder/bundle_sim_result.h"
#include "builder/errors.h"
#include "builder/rest_client.h"
#include "candidate/candidate.h"
#include "common/config.h"

#include <boost/asio/io_context.hpp>

#include <expected>
#include <string>

namespace simulation
{

class Simulator final
{
public:
    Simulator(Config config, boost::asio::io_context& io_ctx);

    std::expected<builder::BundleSimulationResult, builder::Error> simulate(const candidate::Candidate& candidate) const;

private:
    builder::RestClient m_builder_rest_client;
};

} // namespace simulation
