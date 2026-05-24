#pragma once

#include "sandbox_dex.h"

#include "common/types.h"

#include <solabi/decoder.h>

namespace evm
{

struct Pool : public SandboxDex
{
    Pool(bytes_view address, bytes_view token_a, bytes_view token_b)
        : address{address}
        , token_a{token_a}
        , token_b{token_b}
    {}

    bytes address;
    bytes token_a;
    bytes token_b;
};

} // namespace evm
