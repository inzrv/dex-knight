#include "candidate/local_mempool.h"

#include <utility>

namespace candidate
{

bool LocalMempool::apply_snapshot(MempoolSnapshot snapshot)
{
    std::map<uint64_t, PendingTx> next_candidates;
    for (auto& candidate : snapshot.candidates) {
        next_candidates[candidate.seq_num] = std::move(candidate);
    }

    std::lock_guard lock{m_mutex};
    if (m_block_number && snapshot.block_number < *m_block_number) {
        return false;
    }

    if (snapshot.snapshot_seq < m_snapshot_seq) {
        return false;
    }

    m_candidates = std::move(next_candidates);
    m_snapshot_seq = snapshot.snapshot_seq;
    m_block_number = snapshot.block_number;
    return true;
}

bool LocalMempool::apply_pending_tx(PendingTx candidate)
{
    std::lock_guard lock{m_mutex};
    if (candidate.seq_num <= m_snapshot_seq) {
        return false;
    }

    m_candidates[candidate.seq_num] = std::move(candidate);
    return true;
}

std::optional<PendingTx> LocalMempool::pop_next_candidate()
{
    std::lock_guard lock{m_mutex};
    if (m_candidates.empty()) {
        return std::nullopt;
    }

    auto it = m_candidates.begin();
    auto candidate = std::move(it->second);
    m_candidates.erase(it);
    return candidate;
}

size_t LocalMempool::size() const
{
    std::lock_guard lock{m_mutex};
    return m_candidates.size();
}

uint64_t LocalMempool::snapshot_seq() const
{
    std::lock_guard lock{m_mutex};
    return m_snapshot_seq;
}

std::optional<uint64_t> LocalMempool::block_number() const
{
    std::lock_guard lock{m_mutex};
    return m_block_number;
}

} // namespace candidate
