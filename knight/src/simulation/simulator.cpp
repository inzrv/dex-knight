#include "simulation/simulator.h"

#include "builder/bundle.h"

#include <utility>

namespace simulation
{

Simulator::Simulator(Config config, boost::asio::io_context& io_ctx)
    : m_builder_rest_client(std::move(config), io_ctx)
{}

std::expected<std::string, builder::Error> Simulator::simulate(const candidate::Candidate& candidate) const
{
    builder::Bundle bundle;
    bundle.transactions.emplace_back(builder::MempoolTxRef{candidate.tx.mempool_tx_id});

    return m_builder_rest_client.simulate_bundle(bundle);
}

} // namespace simulation
