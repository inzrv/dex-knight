#include "candidate/local_mempool.h"

#include <utility>

namespace candidate
{

bool LocalMempool::apply_snapshot(CandidateSnapshot snapshot)
{
    std::map<uint64_t, Candidate> next_candidates;
    for (auto& candidate : snapshot.candidates) {
        next_candidates.insert_or_assign(candidate.tx.seq_num, std::move(candidate));
    }

    {
        std::lock_guard lock{m_mutex};
        if (snapshot.snapshot_seq < m_snapshot_seq) {
            return false;
        }

        m_candidates = std::move(next_candidates);
        m_snapshot_seq = snapshot.snapshot_seq;
    }

    return true;
}

bool LocalMempool::apply_pending_tx(Candidate candidate)
{
    {
        std::lock_guard lock{m_mutex};
        if (candidate.tx.seq_num <= m_snapshot_seq) {
            return false;
        }

        m_candidates.insert_or_assign(candidate.tx.seq_num, std::move(candidate));
    }

    return true;
}

std::optional<Candidate> LocalMempool::pop_next_candidate()
{
    std::lock_guard lock{m_mutex};
    return pop_next_candidate_locked();
}

void LocalMempool::clear()
{
    {
        std::lock_guard lock{m_mutex};
        m_candidates.clear();
        m_snapshot_seq = 0;
    }
}

std::optional<Candidate> LocalMempool::pop_next_candidate_locked()
{
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

} // namespace candidate
