#pragma once

#include <string_view>

namespace candidate
{

enum class Error
{
};

inline std::string_view error_to_string(Error /* error*/) noexcept
{
    return "UNKNOWN_CANDIDATE_SOURCE_ERROR";
}

} // namespace candidate
