#include "decoder/decoder.h"

#include "test_helpers.h"

#include <gtest/gtest.h>

#include <utility>
#include <variant>

using namespace test;

TEST(DecoderTest, DecodesSwapExactAForB)
{
    const auto input = solabi::from_hex("4cabca0a"
                                        "000000000000000000000000000000000000000000000000000000000000007b"
                                        "000000000000000000000000000000000000000000000000000000000000002d");
    auto candidate = make_candidate(make_pool(), candidate::SwapKind::A_FOR_B, input);

    auto decoded = decoder::decode_swap(candidate);

    ASSERT_TRUE(decoded);
    ASSERT_TRUE(std::holds_alternative<evm::SandboxDex::SwapExactAForB>(*decoded));

    const auto& swap = std::get<evm::SandboxDex::SwapExactAForB>(*decoded);
    EXPECT_TRUE(swap.amount_in == intx::uint256{123});
    EXPECT_TRUE(swap.min_amount_out == intx::uint256{45});
}

TEST(DecoderTest, DecodesSwapExactBForA)
{
    const auto input = solabi::from_hex("dd4f1611"
                                        "00000000000000000000000000000000000000000000000000000000000003db"
                                        "0000000000000000000000000000000000000000000000000000000000000041");
    auto candidate = make_candidate(make_pool(), candidate::SwapKind::B_FOR_A, input);

    auto decoded = decoder::decode_swap(candidate);

    ASSERT_TRUE(decoded);
    ASSERT_TRUE(std::holds_alternative<evm::SandboxDex::SwapExactBForA>(*decoded));

    const auto& swap = std::get<evm::SandboxDex::SwapExactBForA>(*decoded);
    EXPECT_TRUE(swap.amount_in == intx::uint256{987});
    EXPECT_TRUE(swap.min_amount_out == intx::uint256{65});
}

TEST(DecoderTest, RejectsSelectorThatDoesNotMatchCandidateKind)
{
    const auto input = solabi::from_hex("dd4f1611"
                                        "0000000000000000000000000000000000000000000000000000000000000064"
                                        "0000000000000000000000000000000000000000000000000000000000000000");
    auto candidate = make_candidate(make_pool(), candidate::SwapKind::A_FOR_B, input);

    EXPECT_FALSE(decoder::decode_swap(candidate));
}

TEST(DecoderTest, RejectsMalformedCalldata)
{
    const auto input = solabi::from_hex("4cabca0a"
                                        "0000000000000000000000000000000000000000000000000000000000000064");
    auto candidate = make_candidate(make_pool(), candidate::SwapKind::A_FOR_B, input);

    EXPECT_FALSE(decoder::decode_swap(candidate));
}
