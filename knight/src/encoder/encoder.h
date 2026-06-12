#pragma once

#include "common/types.h"
#include "evm/contracts/sandbox_backrun.h"

#include <optional>

namespace encoder
{

std::optional<bytes> encode_execute_backrun(const evm::SandboxBackrun::ExecuteBackrun& call);

} // namespace encoder
