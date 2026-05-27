#include "builder/bundle.h"

namespace builder
{

boost::json::object bundle_item_to_json(const BundleItem& item)
{
    return std::visit(
        [](const auto& value) {
            return value.to_json();
        },
        item);
}

boost::json::object MempoolTxRef::to_json() const
{
    boost::json::object object;
    object["mempoolTxId"] = mempool_tx_id;
    return object;
}

boost::json::object Bundle::to_json() const
{
    boost::json::array items;
    items.reserve(transactions.size());
    for (const auto& transaction : transactions) {
        items.emplace_back(bundle_item_to_json(transaction));
    }

    boost::json::object object;
    object["transactions"] = std::move(items);
    return object;
}

} // namespace builder
