//
// Created by jmarc on 17/6/2021.
//

#ifndef FMS_PROBLEM_INDICES_HPP
#define FMS_PROBLEM_INDICES_HPP

#include "fms/utils/types.hpp"

#include <cstdint>

namespace fms::problem {

/// @class fms::problem::JobId
/// @brief Strong type for the index of a job
STRONG_TYPE(JobId, std::uint32_t);

/// @typedef fms::problem::OperationId
/// @brief Type for the index of an operation within a job
using OperationId = std::uint32_t;

/// @class fms::problem::ModuleId
/// @brief Strong type for the index of a Module
STRONG_TYPE(ModuleId, std::uint32_t);

/// @class fms::problem::MachineId
/// @brief Strong type for the index of a machine
STRONG_TYPE(MachineId, std::uint32_t);

/// @class fms::problem::ReEntrantId
/// @brief Strong type for the index of a re-entrancy
STRONG_TYPE(ReEntrantId, std::uint32_t);

/// @class fms::problem::ReEntrancies
/// @brief Strong type for the number of re-entrancies of a job in a machine (for now the maximum is
/// 256)
STRONG_TYPE(ReEntrancies, std::uint8_t);

namespace plexity {
constexpr ReEntrancies SIMPLEX{1};
constexpr ReEntrancies DUPLEX{2};
} // namespace plexity
} // namespace fms::problem

#endif // FMS_PROBLEM_INDICES_HPP
