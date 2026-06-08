#include "arbitrage/calculator.h"

namespace arbitrage
{

std::optional<Opportunity> find_arbitrage(
    const candidate::StateSnapshot&,
    const candidate::Candidate&,
    const decoder::Swap&,
    const Params&)
{
    return std::nullopt;
}

} // namespace arbitrage
