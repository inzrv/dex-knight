#pragma once

#include "evm/receipt.h"

#include <boost/json.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace builder
{

enum class BundleStatus
{
    INCLUDED,
    FAILED
};

enum class BundleTxStatus
{
    INCLUDED,
    REVERTED,
    MISSING
};

std::optional<BundleStatus> bundle_status_from_string(std::string_view status) noexcept;
std::string_view bundle_status_to_string(BundleStatus status) noexcept;

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

struct BundleResult
{
    BundleStatus status{BundleStatus::FAILED};
    std::vector<BundleTxResult> transactions;

    static std::optional<BundleResult> from_json(const boost::json::value& value);

    boost::json::object to_json() const;
};

struct BundleSimulationResult final : public BundleResult
{
    bool simulated{false};

    static std::optional<BundleSimulationResult> from_json(const boost::json::value& value);

    boost::json::object to_json() const;
};

} // namespace builder
