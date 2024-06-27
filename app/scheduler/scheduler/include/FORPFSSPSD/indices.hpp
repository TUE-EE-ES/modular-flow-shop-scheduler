//
// Created by jmarc on 17/6/2021.
//

#ifndef INDICES_HPP
#define INDICES_HPP

#include "utils/types.hpp"

#include <cstdint>

namespace FORPFSSPSD {

/// Index of a job
STRONG_TYPE(JobId, std::uint32_t);

/// Index of a operation within a job
using OperationId = std::uint32_t;

/// Index of a @ref Module
STRONG_TYPE(ModuleId, std::uint32_t);

/// Index of a machine
STRONG_TYPE(MachineId, std::uint32_t);

/// Index of a re-entrancy
STRONG_TYPE(ReEntrantId, std::uint32_t);

/// Number of re-entrancies of a job in a machine (for now the maximum is 256)
STRONG_TYPE(ReEntrancies, std::uint8_t);
} // namespace FORPFSSPSD

namespace FS = FORPFSSPSD;

#endif // INDICES_HPP
