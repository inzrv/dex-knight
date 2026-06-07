#pragma once

#include "evm/receipt.h"

#include <boost/json.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace builder
{

enum class BundleTxStatus
{
    INCLUDED,
    REVERTED,
    MISSING
};

std::optional<BundleTxStatus> bundle_tx_status_from_string(std::string_view status) noexcept;
std::string_view bundle_tx_status_to_string(BundleTxStatus status) noexcept;

struct BundleTxResult final
{
    std::optional<std::string> mempool_tx_id;
    bytes chain_tx_hash;
    BundleTxStatus status{BundleTxStatus::MISSING};
    std::optional<uint64_t> block_number;
    std::optional<uint64_t> transaction_index;
    std::optional<evm::Receipt> receipt;

    static std::optional<BundleTxResult> from_json(const boost::json::value& value);

    boost::json::object to_json() const;
};

} // namespace builder
