#pragma once

#include "backrun/tx_composer.h"
#include "candidate/candidate_source.h"
#include "common/config.h"
#include "simulation/simulator.h"

#include <boost/asio/io_context.hpp>

#include <memory>

namespace runtime
{

struct RuntimeComponents
{
    std::unique_ptr<candidate::CandidateSource> candidate_source;
    std::unique_ptr<simulation::Simulator> simulator;
    std::unique_ptr<backrun::TxComposer> backrun_tx_composer;
};

class RuntimeFactory final
{
public:
    explicit RuntimeFactory(Config config);

    RuntimeComponents create(boost::asio::io_context& io_ctx);

private:
    Config m_config;
};

} // namespace runtime
