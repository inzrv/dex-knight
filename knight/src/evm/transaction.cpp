#include "evm/transaction.h"

#include "utils/utils.h"

namespace evm
{
std::optional<Transaction> Transaction::from_json(const boost::json::value& value)
{
    if (!value.is_object()) {
        return std::nullopt;
    }

    const auto& object = value.as_object();
    auto from = json_hex_bytes(object, "from", kAddressLength);
    auto input = json_hex_bytes(object, "input");
    auto gas = json_hex_uint64(object, "gas");
    auto nonce = json_hex_uint64(object, "nonce");
    if (!from || !input || !gas || !nonce) {
        return std::nullopt;
    }

    Transaction tx;
    tx.from = std::move(*from);
    tx.input = std::move(*input);
    tx.gas = *gas;
    tx.nonce = *nonce;

    auto value_amount = json_optional_hex_uint256(object, "value");
    auto chain_id = json_optional_hex_uint64(object, "chainId");
    auto type = json_optional_hex_uint64(object, "type");
    if (!value_amount || !chain_id || !type) {
        return std::nullopt;
    }
    if (*value_amount) {
        tx.value = **value_amount;
    }
    if (*chain_id) {
        tx.chain_id = **chain_id;
    }
    if (*type) {
        if (**type > UINT8_MAX) {
            return std::nullopt;
        }
        tx.type = static_cast<uint8_t>(**type);
    }

    auto hash = json_optional_hex_bytes(object, "hash", 32);
    auto to = json_optional_hex_bytes(object, "to", kAddressLength);
    auto gas_price = json_optional_hex_uint256(object, "gasPrice");
    auto max_fee_per_gas = json_optional_hex_uint256(object, "maxFeePerGas");
    auto max_priority_fee_per_gas = json_optional_hex_uint256(object, "maxPriorityFeePerGas");
    if (!hash || !to || !gas_price || !max_fee_per_gas || !max_priority_fee_per_gas) {
        return std::nullopt;
    }

    tx.hash = std::move(*hash);
    tx.to = std::move(*to);
    tx.gas_price = *gas_price;
    tx.max_fee_per_gas = *max_fee_per_gas;
    tx.max_priority_fee_per_gas = *max_priority_fee_per_gas;

    return tx;
}

boost::json::object Transaction::to_json() const
{
    boost::json::object object;

    object["hash"] = nullptr;
    if (hash) {
        object["hash"] = hex_data(*hash);
    }

    object["type"] = hex_quantity(static_cast<uint64_t>(type));

    object["chainId"] = nullptr;
    if (chain_id) {
        object["chainId"] = hex_quantity(*chain_id);
    }

    object["nonce"] = hex_quantity(nonce);
    object["from"] = hex_data(from);

    object["to"] = nullptr;
    if (to) {
        object["to"] = hex_data(*to);
    }

    object["value"] = hex_quantity(value);
    object["gas"] = hex_quantity(gas);

    object["gasPrice"] = nullptr;
    if (gas_price) {
        object["gasPrice"] = hex_quantity(*gas_price);
    }

    object["maxFeePerGas"] = nullptr;
    if (max_fee_per_gas) {
        object["maxFeePerGas"] = hex_quantity(*max_fee_per_gas);
    }

    object["maxPriorityFeePerGas"] = nullptr;
    if (max_priority_fee_per_gas) {
        object["maxPriorityFeePerGas"] = hex_quantity(*max_priority_fee_per_gas);
    }

    object["input"] = hex_data(input);

    return object;
}

} // namespace evm
