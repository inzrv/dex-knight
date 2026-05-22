#include "candidate/local_mempool.h"

#include <utility>

namespace candidate
{

bool LocalMempool::apply_snapshot(builder::PendingSnapshot snapshot)
{
    std::map<uint64_t, builder::PendingTransaction> next_candidates;
    for (auto& candidate : snapshot.transactions) {
        next_candidates[candidate.seq_num] = std::move(candidate);
    }

    {
        std::lock_guard lock{m_mutex};
        if (m_closed) {
            return false;
        }

        if (m_block_number && snapshot.block_number < *m_block_number) {
            return false;
        }

        if (snapshot.snapshot_seq < m_snapshot_seq) {
            return false;
        }

        m_candidates = std::move(next_candidates);
        m_snapshot_seq = snapshot.snapshot_seq;
        m_block_number = snapshot.block_number;
    }

    m_cv.notify_one();
    return true;
}

bool LocalMempool::apply_pending_tx(builder::PendingTransaction candidate)
{
    {
        std::lock_guard lock{m_mutex};
        if (m_closed || candidate.seq_num <= m_snapshot_seq) {
            return false;
        }

        m_candidates[candidate.seq_num] = std::move(candidate);
    }

    m_cv.notify_one();
    return true;
}

std::optional<builder::PendingTransaction> LocalMempool::try_pop_next_candidate()
{
    std::lock_guard lock{m_mutex};
    return pop_next_candidate_locked();
}

std::expected<builder::PendingTransaction, Error> LocalMempool::wait_pop_next_candidate()
{
    std::unique_lock lock{m_mutex};
    m_cv.wait(lock, [this] {
        return !m_candidates.empty() || m_closed;
    });

    auto candidate = pop_next_candidate_locked();
    if (!candidate) {
        return std::unexpected(Error::CLOSED);
    }

    return std::move(*candidate);
}

void LocalMempool::close()
{
    {
        std::lock_guard lock{m_mutex};
        m_closed = true;
    }

    m_cv.notify_all();
}

std::optional<builder::PendingTransaction> LocalMempool::pop_next_candidate_locked()
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

std::optional<uint64_t> LocalMempool::block_number() const
{
    std::lock_guard lock{m_mutex};
    return m_block_number;
}

} // namespace candidate
