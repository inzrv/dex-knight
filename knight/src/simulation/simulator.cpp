#include "simulation/simulator.h"

#include "utils/utils.h"

#include <utility>

namespace simulation
{

Simulator::Simulator(Config config, boost::asio::io_context& io_ctx)
    : m_builder_rest_client(std::move(config), io_ctx)
{}

std::expected<builder::BundleSimulationResult, builder::Error> Simulator::simulate(
    const builder::Bundle& bundle) const
{
    auto raw_response = m_builder_rest_client.simulate_bundle(bundle);
    if (!raw_response) {
        return std::unexpected(raw_response.error());
    }

    auto json = parse_to_json(*raw_response);
    if (!json) {
        return std::unexpected(builder::Error::INVALID_RESPONSE);
    }

    auto result = builder::BundleSimulationResult::from_json(*json);
    if (!result) {
        return std::unexpected(builder::Error::INVALID_RESPONSE);
    }

    return *result;
}

std::expected<builder::BundleResult, builder::Error> Simulator::submit(
    const builder::Bundle& bundle) const
{
    auto raw_response = m_builder_rest_client.submit_bundle(bundle);
    if (!raw_response) {
        return std::unexpected(raw_response.error());
    }

    auto json = parse_to_json(*raw_response);
    if (!json) {
        return std::unexpected(builder::Error::INVALID_RESPONSE);
    }

    auto result = builder::BundleResult::from_json(*json);
    if (!result) {
        return std::unexpected(builder::Error::INVALID_RESPONSE);
    }

    return *result;
}

} // namespace simulation
