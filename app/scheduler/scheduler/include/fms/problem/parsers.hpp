#ifndef FMS_PROBLEM_PARSERS_HPP
#define FMS_PROBLEM_PARSERS_HPP

#include "indices.hpp"
#include "operation.hpp"

namespace fms::problem {

/// @brief Parses a string to a MachineId
/// @param machineIdStr The string to parse
/// @return The parsed MachineId
MachineId parseMachineId(std::string_view machineIdStr);

/// @brief Parses a string to a JobId
/// @param jobIdStr The string to parse
/// @return The parsed JobId
JobId parseJobId(std::string_view jobIdStr);

/// @brief Parses a string to an OperationId
/// @param operationIdStr The string to parse
/// @return The parsed OperationId
OperationId parseOperationId(std::string_view operationIdStr);

/// @brief Parses two strings to an operation
/// @param jobIdStr The string to parse for the JobId
/// @param operationIdStr The string to parse for the OperationId
/// @return The parsed operation
Operation parseOperation(std::string_view jobIdStr, std::string_view operationIdStr);

} // namespace fms::problem

#endif // FMS_PROBLEM_PARSERS_HPP