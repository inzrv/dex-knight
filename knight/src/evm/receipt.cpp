#include "evm/receipt.h"

#include "utils/utils.h"

#include <utility>

namespace evm
{

std::optional<Receipt> Receipt::from_json(const boost::json::value& value)
{
    if (!value.is_object()) {
        return std::nullopt;
    }

    const auto& object = value.as_object();
    const auto type = json_hex_uint64(object, "type");
    const auto status = json_hex_uint64(object, "status");
    const auto cumulative_gas_used = json_hex_uint64(object, "cumulativeGasUsed");
    const auto* raw_logs = json_array(object, "logs");
    const auto logs_bloom = json_hex_bytes(object, "logsBloom", 256);
    const auto transaction_hash = json_hex_bytes(object, "transactionHash", 32);
    const auto transaction_index = json_hex_uint64(object, "transactionIndex");
    const auto block_hash = json_hex_bytes(object, "blockHash", 32);
    const auto block_number = json_hex_uint64(object, "blockNumber");
    const auto gas_used = json_hex_uint64(object, "gasUsed");
    const auto effective_gas_price = json_hex_uint256(object, "effectiveGasPrice");
    const auto from = json_hex_bytes(object, "from", ADDRESS_LENGTH);

    if (!type || *type > UINT8_MAX || !status || !cumulative_gas_used || raw_logs == nullptr || !logs_bloom
        || !transaction_hash || !transaction_index || !block_hash || !block_number || !gas_used
        || !effective_gas_price || !from) {
        return std::nullopt;
    }

    std::vector<Log> logs;
    logs.reserve(raw_logs->size());
    for (const auto& raw_log : *raw_logs) {
        auto log = Log::from_json(raw_log);
        if (!log) {
            return std::nullopt;
        }
        logs.push_back(std::move(*log));
    }

    auto to = json_optional_hex_bytes(object, "to", ADDRESS_LENGTH);
    auto contract_address = json_optional_hex_bytes(object, "contractAddress", ADDRESS_LENGTH);
    auto block_timestamp = json_optional_uint64(object, "blockTimestamp");
    if (!to || !contract_address || !block_timestamp) {
        return std::nullopt;
    }

    Receipt receipt = {
        .type = static_cast<uint8_t>(*type),
        .status = *status,
        .cumulative_gas_used = *cumulative_gas_used,
        .logs = std::move(logs),
        .logs_bloom = std::move(*logs_bloom),
        .transaction_hash = std::move(*transaction_hash),
        .transaction_index = *transaction_index,
        .block_hash = std::move(*block_hash),
        .block_number = *block_number,
        .gas_used = *gas_used,
        .effective_gas_price = *effective_gas_price,
        .from = std::move(*from),
        .to = std::move(*to),
        .contract_address = std::move(*contract_address),
        .block_timestamp = *block_timestamp
    };

    return receipt;
}

boost::json::object Receipt::to_json() const
{
    boost::json::object object;

    object["type"] = hex_quantity(static_cast<uint64_t>(type));
    object["status"] = hex_quantity(status);
    object["cumulativeGasUsed"] = hex_quantity(cumulative_gas_used);
    boost::json::array raw_logs;
    raw_logs.reserve(logs.size());
    for (const auto& log : logs) {
        raw_logs.emplace_back(log.to_json());
    }
    object["logs"] = std::move(raw_logs);
    object["logsBloom"] = hex_data(logs_bloom);
    object["transactionHash"] = hex_data(transaction_hash);
    object["transactionIndex"] = hex_quantity(transaction_index);
    object["blockHash"] = hex_data(block_hash);
    object["blockNumber"] = hex_quantity(block_number);
    object["gasUsed"] = hex_quantity(gas_used);
    object["effectiveGasPrice"] = hex_quantity(effective_gas_price);

    object["from"] = hex_data(from);

    object["to"] = nullptr;
    if (to) {
        object["to"] = hex_data(*to);
    }

    object["contractAddress"] = nullptr;
    if (contract_address) {
        object["contractAddress"] = hex_data(*contract_address);
    }

    object["blockTimestamp"] = nullptr;
    if (block_timestamp) {
        object["blockTimestamp"] = hex_quantity(*block_timestamp);
    }

    return object;
}

} // namespace evm
