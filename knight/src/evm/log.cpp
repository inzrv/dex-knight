#include "evm/log.h"

#include "utils/utils.h"

#include <utility>

namespace evm
{

std::optional<Log> Log::from_json(const boost::json::value& value)
{
    if (!value.is_object()) {
        return std::nullopt;
    }

    const auto& object = value.as_object();
    auto address = json_hex_bytes(object, "address", ADDRESS_LENGTH);
    auto data = json_hex_bytes(object, "data");
    const auto* raw_topics = json_array(object, "topics");

    if (!address || !data || raw_topics == nullptr) {
        return std::nullopt;
    }

    std::vector<bytes> topics;
    topics.reserve(raw_topics->size());
    for (const auto& raw_topic : *raw_topics) {
        if (!raw_topic.is_string()) {
            return std::nullopt;
        }
        auto topic = parse_hex_bytes(raw_topic.as_string().c_str());
        if (!topic || topic->size() != WORD_SIZE) {
            return std::nullopt;
        }
        topics.push_back(std::move(*topic));
    }

    return Log{
        .address = std::move(*address),
        .topics = std::move(topics),
        .data = std::move(*data)
    };
}

boost::json::object Log::to_json() const
{
    boost::json::array raw_topics;
    raw_topics.reserve(topics.size());
    for (const auto& topic : topics) {
        raw_topics.emplace_back(boost::json::string(hex_data(topic)));
    }

    boost::json::object object;
    object["address"] = hex_data(address);
    object["topics"] = std::move(raw_topics);
    object["data"] = hex_data(data);
    return object;
}

} // namespace evm
