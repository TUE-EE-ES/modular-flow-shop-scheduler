//
// Created by jmarc on 17/6/2021.
//

#ifndef INDICES_HPP
#define INDICES_HPP

#include "utils/types.hpp"

#include <cstdint>
#include <fmt/core.h>

namespace FORPFSSPSD {

struct P_ModuleIdTag {};

/// Index of a @ref Module
using ModuleId = FMS::Utils::StrongType<P_ModuleIdTag, std::uint32_t>;

struct P_MachineIdTag {};

/// Index of a machine
using MachineId = FMS::Utils::StrongType<P_MachineIdTag, std::uint32_t>;

struct P_ReEntrantIdTag {};

/// Index of a re-entrancy
using ReEntrantId = FMS::Utils::StrongType<P_ReEntrantIdTag, std::uint32_t>;


struct P_ReEntranciesTag {};

/// Number of re-entrancies of a job in a machine (for now the maximum is 256)
using ReEntrancies = FMS::Utils::StrongType<P_ReEntranciesTag, std::uint8_t>;
} // namespace FORPFSSPSD

#endif // INDICES_HPP
