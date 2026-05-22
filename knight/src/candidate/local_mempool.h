#pragma once

#include "builder/pending_snapshot.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>

namespace candidate
{

class LocalMempool final
{
public:
    bool apply_snapshot(builder::PendingSnapshot snapshot);
    bool apply_pending_tx(builder::PendingTransaction candidate);
    std::optional<builder::PendingTransaction> pop_next_candidate();

    [[nodiscard]] size_t size() const;
    [[nodiscard]] uint64_t snapshot_seq() const;
    [[nodiscard]] std::optional<uint64_t> block_number() const;

private:
    mutable std::mutex m_mutex;
    std::map<uint64_t, builder::PendingTransaction> m_candidates;
    uint64_t m_snapshot_seq{0};
    // Block seen before requesting the snapshot; useful for simulation context,
    // but not an exact snapshot block because the request may be delayed.
    std::optional<uint64_t> m_block_number;
};

} // namespace candidate
