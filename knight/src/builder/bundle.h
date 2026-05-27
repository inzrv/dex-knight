#pragma once

#include "evm/transaction.h"

#include <boost/json.hpp>

#include <string>
#include <variant>
#include <vector>

namespace builder
{

struct MempoolTxRef final
{
    std::string mempool_tx_id;

    boost::json::object to_json() const;
};

using BundleItem = std::variant<MempoolTxRef, evm::Transaction>;

boost::json::object bundle_item_to_json(const BundleItem& item);

struct Bundle final
{
    std::vector<BundleItem> transactions;

    boost::json::object to_json() const;
};

} // namespace builder
