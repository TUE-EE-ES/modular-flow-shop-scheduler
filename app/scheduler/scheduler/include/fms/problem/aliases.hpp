#ifndef FMS_PROBLEM_ALIASES_HPP
#define FMS_PROBLEM_ALIASES_HPP

#include "indices.hpp"
#include "operation.hpp"

#include "fms/delay.hpp"
#include "fms/utils/containers.hpp"

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fms::problem {
// inner types for temporary storage and mappings

/// Index of a maintenance class
using MaintType = uint32_t;

/// @brief Index of a maintenance class
using MaintType = uint32_t;

/// @brief Table containing times per operation
using DefaultOperationsTime = utils::containers::DefaultMap<Operation, delay>;

/// @brief Table containing the sizes of each operation (aka sheet)
using OperationSizes = utils::containers::DefaultMap<Operation, unsigned int>;

/// @brief Table containing times per job
using JobsTime = std::unordered_map<JobId, delay>;

/// @brief Table with time delays between two operations and a default value
using DefaultTimeBetweenOps = utils::containers::DefaultTwoKeyMap<Operation, delay>;

/// @brief Table with the time between two operations
using TimeBetweenOps = utils::containers::TwoKeyMap<Operation, delay>;

/// @brief Relates a job to its plexity
using PlexityTable = std::unordered_map<JobId, std::vector<ReEntrancies>>;

/// @brief Flow of operations in the flow-shop
using OperationFlowVector = std::vector<OperationId>;

/// @brief Flow of operations per machine in the flow-shop
using MachineMapOperationFlowVector = std::unordered_map<MachineId, OperationFlowVector>;

/// @brief Table mapping each operation the machine where it should be processed
using OperationMachineMap = std::unordered_map<Operation, MachineId>;

/// @brief Vector of operation
using OperationsVector = std::vector<Operation>;

/// @brief Set of operations
using OperationsSet = std::unordered_set<Operation>;

/// @brief Table with the operations of each job
using JobOperations = std::map<JobId, OperationsVector>;

/// @brief Table with the operations of each job but in a set
using JobOperationsSet = std::unordered_map<JobId, OperationsSet>;

/// @brief Set of machines
using MachinesSet = std::unordered_set<MachineId>;
} // namespace fms::problem

#endif // FMS_PROBLEM_ALIASES_HPP
