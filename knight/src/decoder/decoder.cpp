#include "decoder.h"

#include "candidate/candidate.h"

#include <solabi/decoder.h>

#include <exception>

namespace decoder
{

static constexpr size_t SELECTOR_SIZE = 4;

std::optional<Swap> decode_swap(const candidate::Candidate& candidate)
{
    const auto& input = candidate.tx.input;

    if (input.size() <= SELECTOR_SIZE) {
        return std::nullopt;
    }

    const bytes_view selector{input.data(), SELECTOR_SIZE};
    const bytes_view calldata{input.data() + SELECTOR_SIZE, input.size() - SELECTOR_SIZE};

    try {
        switch (candidate.swap_kind) {
            case candidate::SwapKind::A_FOR_B: {
                const auto& expected = evm::SandboxDex::SwapExactAForB::selector;
                if (bytes_view(expected.data(), expected.size()) != selector) {
                    return std::nullopt;
                }
                return solabi::decode<evm::SandboxDex::SwapExactAForB>(calldata);
            }
            case candidate::SwapKind::B_FOR_A: {
                const auto& expected = evm::SandboxDex::SwapExactBForA::selector;
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
