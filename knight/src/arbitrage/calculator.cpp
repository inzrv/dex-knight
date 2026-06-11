#include "arbitrage/calculator.h"

#include "utils/utils.h"

#include <algorithm>
#include <utility>
#include <variant>

namespace arbitrage
{
namespace
{

constexpr uint16_t kBpsDenominator{10'000};

struct RouteQuote final
{
    intx::uint256 amount_in_b{};
    intx::uint256 amount_out_a{};
    intx::uint256 amount_out_b{};
};

std::optional<intx::uint256> amount_out(const intx::uint256& amount_in,
                                        const intx::uint256& reserve_in,
                                        const intx::uint256& reserve_out)
{
    if (amount_in == 0 || reserve_in == 0 || reserve_out == 0) {
        return std::nullopt;
    }

    const auto amount_in_with_fee = amount_in * evm::SandboxDex::kFeeNumerator;
    const auto denominator = reserve_in * evm::SandboxDex::kFeeDenominator + amount_in_with_fee;
    if (denominator == 0) {
        return std::nullopt;
    }

    return (amount_in_with_fee * reserve_out) / denominator;
}

intx::uint256 apply_bps(const intx::uint256& value, uint16_t bps)
{
    return value * intx::uint256{bps} / intx::uint256{kBpsDenominator};
}

const candidate::PoolSnapshot* find_pool_snapshot(const candidate::StateSnapshot& state,
                                                  const candidate::Candidate& candidate)
{
    const auto it = std::find_if(
        state.pools.begin(), state.pools.end(), [&candidate](const auto& pool_snapshot) {
            return pool_snapshot.pool.address == candidate.pool.address;
        });

    return it == state.pools.end() ? nullptr : &*it;
}

std::optional<candidate::PoolSnapshot> apply_victim_swap(
    const candidate::PoolSnapshot& pool_snapshot, const decoder::Swap& swap)
{
    if (std::holds_alternative<evm::SandboxDex::SwapExactAForB>(swap)) {
        const auto& decoded_swap = std::get<evm::SandboxDex::SwapExactAForB>(swap);
        auto amount_out_b =
            amount_out(decoded_swap.amount_in, pool_snapshot.reserve_a, pool_snapshot.reserve_b);
        if (!amount_out_b || *amount_out_b < decoded_swap.min_amount_out ||
            pool_snapshot.reserve_b < *amount_out_b) {
            return std::nullopt;
        }

        return candidate::PoolSnapshot{
            .pool = pool_snapshot.pool,
            .reserve_a = pool_snapshot.reserve_a + decoded_swap.amount_in,
            .reserve_b = pool_snapshot.reserve_b - *amount_out_b,
        };
    }

    if (std::holds_alternative<evm::SandboxDex::SwapExactBForA>(swap)) {
        const auto& decoded_swap = std::get<evm::SandboxDex::SwapExactBForA>(swap);
        auto amount_out_a =
            amount_out(decoded_swap.amount_in, pool_snapshot.reserve_b, pool_snapshot.reserve_a);
        if (!amount_out_a || *amount_out_a < decoded_swap.min_amount_out ||
            pool_snapshot.reserve_a < *amount_out_a) {
            return std::nullopt;
        }

        return candidate::PoolSnapshot{
            .pool = pool_snapshot.pool,
            .reserve_a = pool_snapshot.reserve_a - *amount_out_a,
            .reserve_b = pool_snapshot.reserve_b + decoded_swap.amount_in,
        };
    }

    return std::nullopt;
}

std::optional<RouteQuote> quote_route(const candidate::PoolSnapshot& buy_pool,
                                      const candidate::PoolSnapshot& sell_pool,
                                      const intx::uint256& amount_in_b)
{
    auto amount_out_a = amount_out(amount_in_b, buy_pool.reserve_b, buy_pool.reserve_a);
    if (!amount_out_a) {
        return std::nullopt;
    }

    auto amount_out_b = amount_out(*amount_out_a, sell_pool.reserve_a, sell_pool.reserve_b);
    if (!amount_out_b) {
        return std::nullopt;
    }

    return RouteQuote{
        .amount_in_b = amount_in_b,
        .amount_out_a = *amount_out_a,
        .amount_out_b = *amount_out_b,
    };
}

std::optional<intx::uint256> optimal_amount_in_b(const candidate::PoolSnapshot& buy_pool,
                                                 const candidate::PoolSnapshot& sell_pool,
                                                 const Params& params)
{
    const intx::uint<1024> kFeeNumerator{evm::SandboxDex::kFeeNumerator};
    const intx::uint<1024> kFeeDenominator{evm::SandboxDex::kFeeDenominator};
    const auto kFeeNumeratorSquared = kFeeNumerator * kFeeNumerator;
    const auto kFeeDenominatorSquared = kFeeDenominator * kFeeDenominator;

    // For B -> A -> B routes, amountOutB(x) = C*x / (D + E*x).
    // The unconstrained continuous optimum is (sqrt(C*D) - D) / E.
    const intx::uint<1024> c = kFeeNumeratorSquared * intx::uint<1024>{buy_pool.reserve_a} *
                               intx::uint<1024>{sell_pool.reserve_b};
    const intx::uint<1024> d = kFeeDenominatorSquared * intx::uint<1024>{buy_pool.reserve_b} *
                               intx::uint<1024>{sell_pool.reserve_a};
    const intx::uint<1024> e =
        kFeeNumerator * kFeeDenominator * intx::uint<1024>{sell_pool.reserve_a} +
        kFeeNumeratorSquared * intx::uint<1024>{buy_pool.reserve_a};

    if (c == 0 || d == 0 || e == 0) {
        return std::nullopt;
    }

    const auto root = integer_sqrt(intx::uint<2048>{c} * intx::uint<2048>{d});
    if (root <= intx::uint<2048>{d}) {
        return std::nullopt;
    }

    auto amount_in_b = (root - intx::uint<2048>{d}) / intx::uint<2048>{e};
    if (amount_in_b == 0) {
        return std::nullopt;
    }

    const intx::uint<2048> max_amount_in_b{params.max_amount_in_b};
    if (amount_in_b > max_amount_in_b) {
        amount_in_b = max_amount_in_b;
    }

    return intx::uint256{amount_in_b};
}

std::optional<RouteQuote> best_route_quote(const candidate::PoolSnapshot& buy_pool,
                                           const candidate::PoolSnapshot& sell_pool,
                                           const Params& params)
{
    const auto optimal_amount = optimal_amount_in_b(buy_pool, sell_pool, params);
    if (!optimal_amount) {
        return std::nullopt;
    }

    return quote_route(buy_pool, sell_pool, *optimal_amount);
}

std::optional<Opportunity> evaluate_pool(const candidate::PoolSnapshot& buy_pool,
                                         const candidate::PoolSnapshot& sell_pool,
                                         const Params& params)
{
    auto quote = best_route_quote(buy_pool, sell_pool, params);
    if (!quote || quote->amount_out_b <= quote->amount_in_b) {
        return std::nullopt;
    }

    const auto profit_b = quote->amount_out_b - quote->amount_in_b;
    if (profit_b < params.min_profit_b) {
        return std::nullopt;
    }

    return Opportunity{
        .buy_pool = buy_pool.pool,
        .sell_pool = sell_pool.pool,
        .amount_in_b = quote->amount_in_b,
        .min_amount_out_a = apply_bps(quote->amount_out_a, params.min_output_bps),
        .min_amount_out_b = apply_bps(quote->amount_out_b, params.min_output_bps),
        .min_profit_b = params.min_profit_b,
        .expected_amount_out_a = quote->amount_out_a,
        .expected_amount_out_b = quote->amount_out_b,
        .expected_profit_b = profit_b,
    };
}

bool is_better(const Opportunity& lhs, const Opportunity& rhs)
{
    return lhs.expected_profit_b > rhs.expected_profit_b;
}

} // namespace

std::optional<Opportunity> find_arbitrage(const candidate::StateSnapshot& state,
                                          const candidate::Candidate& candidate,
                                          const decoder::Swap& swap,
                                          const Params& params)
{
    if (params.min_output_bps > kBpsDenominator) {
        return std::nullopt;
    }

    if (params.max_amount_in_b == 0) {
        return std::nullopt;
    }

    const auto* pool_snapshot = find_pool_snapshot(state, candidate);
    if (!pool_snapshot) {
        return std::nullopt;
    }

    const auto pool_after_victim = apply_victim_swap(*pool_snapshot, swap);
    if (!pool_after_victim) {
        return std::nullopt;
    }

    std::optional<Opportunity> best_opportunity;
    for (const auto& other_pool : state.pools) {
        if (other_pool.pool.address == candidate.pool.address) {
            continue;
        }

        std::optional<Opportunity> opportunity;
        if (std::holds_alternative<evm::SandboxDex::SwapExactAForB>(swap)) {
            opportunity = evaluate_pool(*pool_after_victim, other_pool, params);
        } else {
            opportunity = evaluate_pool(other_pool, *pool_after_victim, params);
        }

        if (opportunity && (!best_opportunity || is_better(*opportunity, *best_opportunity))) {
            best_opportunity = std::move(opportunity);
        }
    }

    return best_opportunity;
}

} // namespace arbitrage
