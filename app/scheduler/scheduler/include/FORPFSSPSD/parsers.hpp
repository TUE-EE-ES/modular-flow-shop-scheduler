#ifndef FORPFSSPSD_PARSERS_HPP
#define FORPFSSPSD_PARSERS_HPP

#include "FORPFSSPSD/indices.hpp"
#include "FORPFSSPSD/operation.h"

namespace FORPFSSPSD {
MachineId parseMachineId(std::string_view machineIdStr);
JobId parseJobId(std::string_view jobIdStr);
OperationId parseOperationId(std::string_view operationIdStr);
operation parseOperation(std::string_view jobIdStr, std::string_view operationIdStr);
} // namespace FORPFSSPSD

#endif