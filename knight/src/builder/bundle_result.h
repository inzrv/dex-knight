#pragma once

#include "builder/bundle_tx_result.h"

#include <boost/json.hpp>

#include <optional>
#include <string_view>
#include <vector>

namespace builder
{

enum class BundleStatus
{
    INCLUDED,
    FAILED
};

std::optional<BundleStatus> bundle_status_from_string(std::string_view status) noexcept;
std::string_view bundle_status_to_string(BundleStatus status) noexcept;

struct BundleResult
{
    BundleStatus status{BundleStatus::FAILED};
    std::vector<BundleTxResult> transactions;

    static std::optional<BundleResult> from_json(const boost::json::value& value);

    boost::json::object to_json() const;
};

} // namespace builder
