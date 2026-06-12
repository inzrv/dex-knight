#include "decoder.h"

#include "candidate/candidate.h"

#include <solabi/decoder.h>

#include <exception>

namespace decoder
{

std::optional<Swap> decode_swap(const candidate::Candidate& candidate)
{
    const auto& input = candidate.tx.input;

    if (input.size() <= kSelectorLength) {
        return std::nullopt;
    }

    const bytes_view selector{input.data(), kSelectorLength};
    const bytes_view calldata{input.data() + kSelectorLength, input.size() - kSelectorLength};

    try {
        switch (candidate.swap_kind) {
        case candidate::SwapKind::A_FOR_B: {
            const auto& expected = evm::SandboxDex::SwapExactAForB::kSelector;
            if (bytes_view(expected.data(), expected.size()) != selector) {
                return std::nullopt;
            }
            return solabi::decode<evm::SandboxDex::SwapExactAForB>(calldata);
        }
        case candidate::SwapKind::B_FOR_A: {
            const auto& expected = evm::SandboxDex::SwapExactBForA::kSelector;
            if (bytes_view(expected.data(), expected.size()) != selector) {
                return std::nullopt;
            }
            return solabi::decode<evm::SandboxDex::SwapExactBForA>(calldata);
        }
        }
    } catch (const std::exception& /*e*/) {
        return std::nullopt;
    }

    return std::nullopt;
}

} // namespace decoder
