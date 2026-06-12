#include "encoder.h"

#include <algorithm>
#include <intx/intx.hpp>
#include <span>

namespace encoder
{
namespace
{

void append_uint256_word(bytes& out, const intx::uint256& value)
{
    const auto offset = out.size();
    out.resize(offset + kWordSize, uint8_t{0});
    intx::be::store(std::span<uint8_t, kWordSize>{out.data() + offset, kWordSize}, value);
}

bool append_address_word(bytes& out, const bytes& address)
{
    if (address.size() != kAddressLength) {
        return false;
    }

    const auto offset = out.size();
    out.resize(offset + kWordSize, uint8_t{0});
    std::copy(address.begin(), address.end(), out.begin() + offset + kWordSize - kAddressLength);
    return true;
}

} // namespace

std::optional<bytes> encode_execute_backrun(const evm::SandboxBackrun::ExecuteBackrun& call)
{
    bytes calldata = evm::SandboxBackrun::ExecuteBackrun::kSelector;

    if (!append_address_word(calldata, call.buy_pool) ||
        !append_address_word(calldata, call.sell_pool)) {
        return std::nullopt;
    }

    append_uint256_word(calldata, call.amount_in_b);
    append_uint256_word(calldata, call.min_amount_out_a);
    append_uint256_word(calldata, call.min_amount_out_b);
    append_uint256_word(calldata, call.min_profit_b);

    return calldata;
}

} // namespace encoder
