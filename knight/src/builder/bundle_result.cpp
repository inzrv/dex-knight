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
    case BundleStatus::INCLUDED:
        return "included";
    case BundleStatus::FAILED:
        return "failed";
    }

    return "unknown";
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

} // namespace builder
