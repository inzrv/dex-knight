#pragma once

#include "evm/contracts/sandbox_dex.h"
#include "candidate/candidate.h"

#include <optional>
#include <variant>

namespace decoder
{

using Swap = std::variant<evm::SandboxDex::SwapExactAForB, evm::SandboxDex::SwapExactBForA>;

std::optional<Swap> decode_swap(const candidate::Candidate& candidate);

} // namespace decoder
