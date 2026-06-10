#include "arbitrage/calculator.h"

#include "test_helpers.h"

#include <gtest/gtest.h>

using namespace test;

TEST(ArbitrageCalculatorTest, FindsAForBBackrunBuyingInVictimPool)
{
    const auto victim_pool = make_pool(kPool1Address);
    const auto other_pool = make_pool(kPool2Address);
    const auto candidate = make_candidate(victim_pool, candidate::SwapKind::A_FOR_B);
    const auto state = make_state(victim_pool, other_pool, candidate);
    const decoder::Swap swap = evm::SandboxDex::SwapExactAForB{
        .amount_in = tokens(100),
        .min_amount_out = intx::uint256{0},
    };

    auto opportunity = arbitrage::find_arbitrage(state, candidate, swap, permissive_params());

    ASSERT_TRUE(opportunity);
    EXPECT_EQ(opportunity->buy_pool.address, victim_pool.address);
    EXPECT_EQ(opportunity->sell_pool.address, other_pool.address);
    EXPECT_TRUE(opportunity->amount_in_b > intx::uint256{0});
    EXPECT_TRUE(opportunity->expected_amount_out_a > intx::uint256{0});
    EXPECT_TRUE(opportunity->expected_amount_out_b > opportunity->amount_in_b);
    EXPECT_TRUE(opportunity->expected_profit_b > intx::uint256{0});
    EXPECT_TRUE(opportunity->min_amount_out_a == intx::uint256{0});
    EXPECT_TRUE(opportunity->min_amount_out_b == intx::uint256{0});
}

TEST(ArbitrageCalculatorTest, FindsBForABackrunSellingInVictimPool)
{
    const auto victim_pool = make_pool(kPool1Address);
    const auto other_pool = make_pool(kPool2Address);
    const auto candidate = make_candidate(victim_pool, candidate::SwapKind::B_FOR_A);
    const auto state = make_state(victim_pool, other_pool, candidate);
    const decoder::Swap swap = evm::SandboxDex::SwapExactBForA{
        .amount_in = tokens(100),
        .min_amount_out = intx::uint256{0},
    };

    auto opportunity = arbitrage::find_arbitrage(state, candidate, swap, permissive_params());

    ASSERT_TRUE(opportunity);
    EXPECT_EQ(opportunity->buy_pool.address, other_pool.address);
    EXPECT_EQ(opportunity->sell_pool.address, victim_pool.address);
    EXPECT_TRUE(opportunity->amount_in_b > intx::uint256{0});
    EXPECT_TRUE(opportunity->expected_amount_out_b > opportunity->amount_in_b);
    EXPECT_TRUE(opportunity->expected_profit_b > intx::uint256{0});
}

TEST(ArbitrageCalculatorTest, RespectsMaxAmountInB)
{
    const auto victim_pool = make_pool(kPool1Address);
    const auto other_pool = make_pool(kPool2Address);
    const auto candidate = make_candidate(victim_pool, candidate::SwapKind::A_FOR_B);
    const auto state = make_state(victim_pool, other_pool, candidate);
    const decoder::Swap swap = evm::SandboxDex::SwapExactAForB{
        .amount_in = tokens(100),
        .min_amount_out = intx::uint256{0},
    };
    auto params = permissive_params();
    params.max_amount_in_b = tokens(1);

    auto opportunity = arbitrage::find_arbitrage(state, candidate, swap, params);

    ASSERT_TRUE(opportunity);
    EXPECT_TRUE(opportunity->amount_in_b == tokens(1));
}

TEST(ArbitrageCalculatorTest, RejectsInvalidParams)
{
    const auto victim_pool = make_pool(kPool1Address);
    const auto other_pool = make_pool(kPool2Address);
    const auto candidate = make_candidate(victim_pool, candidate::SwapKind::A_FOR_B);
    const auto state = make_state(victim_pool, other_pool, candidate);
    const decoder::Swap swap = evm::SandboxDex::SwapExactAForB{
        .amount_in = tokens(100),
        .min_amount_out = intx::uint256{0},
    };
    auto params = permissive_params();
    params.min_output_bps = 10'001;

    EXPECT_FALSE(arbitrage::find_arbitrage(state, candidate, swap, params));

    params = permissive_params();
    params.max_amount_in_b = intx::uint256{0};

    EXPECT_FALSE(arbitrage::find_arbitrage(state, candidate, swap, params));
}

TEST(ArbitrageCalculatorTest, RejectsWhenNoOtherPoolExists)
{
    const auto victim_pool = make_pool(kPool1Address);
    const auto candidate = make_candidate(victim_pool, candidate::SwapKind::A_FOR_B);
    const candidate::StateSnapshot state{
        .valid = true,
        .block_number = 1,
        .pools = {{.pool = victim_pool, .reserve_a = tokens(1'000), .reserve_b = tokens(1'000)}},
        .candidate = candidate,
    };
    const decoder::Swap swap = evm::SandboxDex::SwapExactAForB{
        .amount_in = tokens(100),
        .min_amount_out = intx::uint256{0},
    };

    EXPECT_FALSE(arbitrage::find_arbitrage(state, candidate, swap, permissive_params()));
}

TEST(ArbitrageCalculatorTest, RejectsWhenMinimumProfitIsTooHigh)
{
    const auto victim_pool = make_pool(kPool1Address);
    const auto other_pool = make_pool(kPool2Address);
    const auto candidate = make_candidate(victim_pool, candidate::SwapKind::A_FOR_B);
    const auto state = make_state(victim_pool, other_pool, candidate);
    const decoder::Swap swap = evm::SandboxDex::SwapExactAForB{
        .amount_in = tokens(100),
        .min_amount_out = intx::uint256{0},
    };
    auto params = permissive_params();
    params.min_profit_b = tokens(1'000);

    EXPECT_FALSE(arbitrage::find_arbitrage(state, candidate, swap, params));
}
