#include "builder/bundle_sim_result.h"

#include "utils/utils.h"

#include <utility>

namespace builder
{

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
