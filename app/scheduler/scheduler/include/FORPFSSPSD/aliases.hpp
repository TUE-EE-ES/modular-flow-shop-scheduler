#ifndef ALIASES_HPP
#define ALIASES_HPP

#include "FORPFSSPSD/indices.hpp"
#include "FORPFSSPSD/operation.h"
#include "delay.h"
#include "utils/default_map.hpp"

#include "pch/containers.hpp"

namespace FORPFSSPSD {
// inner types for temporary storage and mappings

/// Index of a maintenance class
using MaintType = uint32_t;

// enum class ShopType { FLOWSHOP, JOBSHOP, FIXEDORDERSHOP};

/// Table containing times per operation
using DefaultOperationsTime = FMS::DefaultMap<operation, delay>;

/// Table containing times per job
using JobsTime = std::unordered_map<JobId, delay>;

/// Table with time delays between two operations and a default value
using DefaultTimeBetweenOps = FMS::DefaultTwoKeyMap<operation, delay>;

/// Table with the time between two operations
using TimeBetweenOps = FMS::TwoKeyMap<operation, delay>;

/// Relates a job to its plexity
using PlexityTable = std::unordered_map<JobId, std::vector<ReEntrancies>>;

/// Flow of operations in the flow-shop
using OperationFlowVector = std::vector<OperationId>;

/// Flow of machines in the flow-shop
using MachineFlowVector = std::vector<MachineId>;

/// Flow of operations per machine in the flow-shop
using MachineMapOperationFlowVector = std::unordered_map<MachineId, OperationFlowVector>;

/// Table mapping each operation the machine where it should be processed
using OperationMachineMap = std::unordered_map<operation, MachineId>;

/// Vector of operation
using OperationsVector = std::vector<operation>;

/// Set of operations
using OperationsSet = std::unordered_set<operation>;

/// Table with the operations of each job
using JobOperations = std::map<JobId, OperationsVector>;
// using JobOperations = std::unordered_map<JobId, OperationsVector>;

/// Table with the operations of each job but in a set
using JobOperationsSet = std::unordered_map<JobId, OperationsSet>;

/// Mapping between a job and its output order
using JobOutMap = std::unordered_map<JobId, JobOutOrder>;

/// Set of machines
using MachinesSet = std::unordered_set<MachineId>;
} // namespace FORPFSSPSD

#endif