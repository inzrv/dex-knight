#pragma once

#include "builder/bundle_result.h"

#include <boost/json.hpp>

#include <optional>

namespace builder
{

struct BundleSimulationResult final : public BundleResult
{
    bool simulated{false};

    static std::optional<BundleSimulationResult> from_json(const boost::json::value& value);

    boost::json::object to_json() const;
};

} // namespace builder
