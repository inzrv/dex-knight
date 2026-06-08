#pragma once

#include "arbitrage/opportunity.h"
#include "arbitrage/params.h"
#include "candidate/candidate.h"
#include "candidate/state.h"
#include "decoder/decoder.h"

#include <optional>

namespace arbitrage
{

[[nodiscard]] std::optional<Opportunity> find_arbitrage(
    const candidate::StateSnapshot& state,
    const candidate::Candidate& candidate,
    const decoder::Swap& swap,
    const Params& params);

} // namespace arbitrage
