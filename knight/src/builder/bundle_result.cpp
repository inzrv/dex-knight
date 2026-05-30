#include "builder/bundle_result.h"

#include "utils/utils.h"

#include <utility>

namespace builder
{

std::optional<BundleStatus> bundle_status_from_string(std::string_view status) noexcept
{
    if (status == "included") {
        return BundleStatus::INCLUDED;
    }
    if (status == "failed") {
        return BundleStatus::FAILED;
    }

    return std::nullopt;
}

std::string_view bundle_status_to_string(BundleStatus status) noexcept
{
    switch (status) {
        case BundleStatus::INCLUDED: return "included";
        case BundleStatus::FAILED: return "failed";
    }

    return "unknown";
}

std::optional<BundleTxStatus> bundle_tx_status_from_string(std::string_view status) noexcept
{
    if (status == "included") {
        return BundleTxStatus::INCLUDED;
    }
    if (status == "reverted") {
        return BundleTxStatus::REVERTED;
    }
    if (status == "missing") {
        return BundleTxStatus::MISSING;
    }

    return std::nullopt;
}

std::string_view bundle_tx_status_to_string(BundleTxStatus status) noexcept
{
    switch (status) {
        case BundleTxStatus::INCLUDED: return "included";
        case BundleTxStatus::REVERTED: return "reverted";
        case BundleTxStatus::MISSING: return "missing";
    }

    return "unknown";
}

std::optional<BundleTxResult> BundleTxResult::from_json(const boost::json::value& value)
{
    if (!value.is_object()) {
        return std::nullopt;
    }

    const auto& object = value.as_object();
    auto mempool_tx_id = json_optional_string(object, "mempoolTxId");
    auto chain_tx_hash = json_hex_bytes(object, "chainTxHash", 32);
    auto raw_status = json_string(object, "status");
    auto block_number = json_optional_hex_uint64(object, "blockNumber");
    auto transaction_index = json_optional_hex_uint64(object, "transactionIndex");

    if (!mempool_tx_id || !chain_tx_hash || !raw_status || !block_number || !transaction_index) {
        return std::nullopt;
    }

    auto status = bundle_tx_status_from_string(*raw_status);
    if (!status) {
        return std::nullopt;
    }

    const auto* receipt_value = object.if_contains("receipt");
    if (receipt_value == nullptr) {
        return std::nullopt;
    }

    std::optional<evm::Receipt> receipt;
    if (!receipt_value->is_null()) {
        auto parsed_receipt = evm::Receipt::from_json(*receipt_value);
        if (!parsed_receipt) {
            return std::nullopt;
        }
        receipt = std::move(*parsed_receipt);
    }

    BundleTxResult result = {
        .mempool_tx_id = std::move(*mempool_tx_id),
        .chain_tx_hash = std::move(*chain_tx_hash),
        .status = *status,
        .block_number = *block_number,
        .transaction_index = *transaction_index,
        .receipt = std::move(receipt)
    };

    return result;
}

boost::json::object BundleTxResult::to_json() const
{
    boost::json::object object;

    object["mempoolTxId"] = nullptr;
    if (mempool_tx_id) {
        object["mempoolTxId"] = *mempool_tx_id;
    }

    object["chainTxHash"] = hex_data(chain_tx_hash);
    object["status"] = std::string(bundle_tx_status_to_string(status));

    if (block_number) {
        object["blockNumber"] = hex_quantity(*block_number);
    }
    if (transaction_index) {
        object["transactionIndex"] = hex_quantity(*transaction_index);
    }

    object["receipt"] = nullptr;
    if (receipt) {
        object["receipt"] = receipt->to_json();
    }

    return object;
}

std::optional<BundleResult> BundleResult::from_json(const boost::json::value& value)
{
    if (!value.is_object()) {
        return std::nullopt;
    }

    const auto& object = value.as_object();
    auto raw_status = json_string(object, "status");
    const auto* raw_transactions = json_array(object, "transactions");
    if (!raw_status || raw_transactions == nullptr) {
        return std::nullopt;
    }

    auto status = bundle_status_from_string(*raw_status);
    if (!status) {
        return std::nullopt;
    }

    BundleResult result;
    result.status = *status;
    result.transactions.reserve(raw_transactions->size());
    for (const auto& raw_transaction : *raw_transactions) {
        auto transaction = BundleTxResult::from_json(raw_transaction);
        if (!transaction) {
            return std::nullopt;
        }
        result.transactions.push_back(std::move(*transaction));
    }

    return result;
}

boost::json::object BundleResult::to_json() const
{
    boost::json::array raw_transactions;
    raw_transactions.reserve(transactions.size());
    for (const auto& transaction : transactions) {
        raw_transactions.emplace_back(transaction.to_json());
    }

    boost::json::object object;
    object["status"] = std::string(bundle_status_to_string(status));
    object["transactions"] = std::move(raw_transactions);
    return object;
}

std::optional<BundleSimulationResult> BundleSimulationResult::from_json(const boost::json::value& value)
{
    if (!value.is_object()) {
        return std::nullopt;
    }

    auto bundle_result = BundleResult::from_json(value);
    if (!bundle_result) {
        return std::nullopt;
    }

    const auto& object = value.as_object();
    auto simulated = json_bool(object, "simulated");
    if (!simulated) {
        return std::nullopt;
    }

    BundleSimulationResult result;
    result.status = bundle_result->status;
    result.transactions = std::move(bundle_result->transactions);
    result.simulated = *simulated;

    return result;
}

boost::json::object BundleSimulationResult::to_json() const
{
    auto object = BundleResult::to_json();
    object["simulated"] = simulated;
    return object;
}

} // namespace builder
