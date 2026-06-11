#include "candidate/pool_snapshot.h"

#include "evm/contracts/sandbox_dex.h"
#include "utils/utils.h"

#include <solabi/decoder.h>

#include <exception>

namespace candidate
{

std::optional<PoolSnapshot> PoolSnapshot::from_json(const evm::Pool& pool,
                                                    const boost::json::value& value)
{
    if (!value.is_object()) {
        return std::nullopt;
    }

    const auto& object = value.as_object();
    const auto raw_result = json_string(object, "result");
    if (!raw_result) {
        return std::nullopt;
    }

    const auto result = parse_hex_bytes(*raw_result);
    if (!result) {
        return std::nullopt;
    }

    try {
        const auto reserves = solabi::decode<evm::SandboxDex::GetReserves>(*result);
        return PoolSnapshot{
            .pool = pool,
            .reserve_a = reserves.reserve_a,
            .reserve_b = reserves.reserve_b,
        };
    } catch (const std::exception& /*e*/) {
        return std::nullopt;
    }
}

} // namespace candidate
