#include "candidate/candidate_filter.h"

#include "evm/contracts/sandbox_dex.h"

#include <algorithm>
#include <utility>

namespace candidate
{
namespace
{

bool starts_with_selector(const bytes& input, const bytes& selector)
{
    return input.size() >= selector.size() && std::equal(selector.begin(), selector.end(), input.begin());
}

std::optional<SwapKind> swap_kind_from_input(const bytes& input)
{
    if (starts_with_selector(input, evm::SandboxDex::SwapExactAForB::selector)) {
        return SwapKind::A_FOR_B;
    }

    if (starts_with_selector(input, evm::SandboxDex::SwapExactBForA::selector)) {
        return SwapKind::B_FOR_A;
    }

    return std::nullopt;
}

} // namespace

CandidateFilter::CandidateFilter(std::vector<evm::Pool> pools)
    : m_pools(std::move(pools))
{}

std::optional<Candidate> CandidateFilter::filter(builder::PendingTransaction tx) const
{
    if (!tx.to) {
        return std::nullopt;
    }

    const auto* pool = find_pool(*tx.to);
    if (pool == nullptr) {
        return std::nullopt;
    }

    auto swap_kind = swap_kind_from_input(tx.input);
    if (!swap_kind) {
        return std::nullopt;
    }

    return Candidate{
        .tx = std::move(tx),
        .pool = *pool,
        .swap_kind = *swap_kind,
    };
}

CandidateSnapshot CandidateFilter::filter_snapshot(builder::PendingSnapshot snapshot) const
{
    CandidateSnapshot filtered{
        .block_number = snapshot.block_number,
        .snapshot_seq = snapshot.snapshot_seq,
    };

    for (auto& tx : snapshot.transactions) {
        auto candidate = filter(std::move(tx));
        if (candidate) {
            filtered.candidates.push_back(std::move(*candidate));
        }
    }

    return filtered;
}

const evm::Pool* CandidateFilter::find_pool(const bytes& address) const
{
    const auto it = std::find_if(m_pools.begin(), m_pools.end(), [&address](const evm::Pool& pool) {
        return pool.address == address;
    });

    return it == m_pools.end() ? nullptr : &*it;
}

} // namespace candidate
