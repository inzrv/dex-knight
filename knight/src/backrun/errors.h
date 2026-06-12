#pragma once

#include <string_view>

namespace backrun
{

enum class Error
{
    NONCE_REQUEST_FAILED,
    NOT_INITIALIZED,
    CALL_ENCODING_FAILED
};

inline std::string_view error_to_string(Error error) noexcept
{
    switch (error) {
    case Error::NONCE_REQUEST_FAILED:
        return "NONCE_REQUEST_FAILED";
    case Error::NOT_INITIALIZED:
        return "NOT_INITIALIZED";
    case Error::CALL_ENCODING_FAILED:
        return "CALL_ENCODING_FAILED";
    }

    return "UNKNOWN_BACKRUN_ERROR";
}

} // namespace backrun
