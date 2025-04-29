#include "fms/pch/containers.hpp"

#include "fms/problem/parsers.hpp"

#include "fms/scheduler_exception.hpp"

using namespace fms;
using namespace fms::problem;

namespace {
void throwWrongMachineId(std::string_view machineId) {
    LOG_E("The machine {} is not valid", machineId);
    throw FmsSchedulerException("The machine is not valid");
}

void throwWrongJobId(std::string_view jobId) {
    LOG_E("The job {} is not valid", jobId);
    throw FmsSchedulerException("The job is not valid");
}

void throwWrongOperationId(std::string_view operationId) {
    LOG_E("The operation {} is not valid", operationId);
    throw FmsSchedulerException("The operation is not valid");
}

} // namespace

MachineId problem::parseMachineId(std::string_view machineIdStr) {
    try {
        return static_cast<MachineId>(std::stoi(std::string(machineIdStr)));
    } catch (const std::out_of_range &e) {
        throwWrongMachineId(machineIdStr);
    } catch (const std::invalid_argument &e) {
        throwWrongMachineId(machineIdStr);
    }

    // Impossible to reach
    return static_cast<MachineId>(0);
}

JobId problem::parseJobId(std::string_view jobIdStr) {
    try {
        return static_cast<JobId>(std::stoi(std::string(jobIdStr)));
    } catch (const std::out_of_range &e) {
        throwWrongJobId(jobIdStr);
    } catch (const std::invalid_argument &e) {
        throwWrongJobId(jobIdStr);
    }

    // Impossible to reach
    return static_cast<JobId>(0);
}

OperationId problem::parseOperationId(std::string_view operationIdStr) {
    try {
        return static_cast<OperationId>(std::stoi(std::string(operationIdStr)));
    } catch (const std::out_of_range &e) {
        throwWrongOperationId(operationIdStr);
    } catch (const std::invalid_argument &e) {
        throwWrongOperationId(operationIdStr);
    }

    // Impossible to reach
    return static_cast<OperationId>(0);
}

Operation problem::parseOperation(std::string_view jobIdStr, std::string_view operationIdStr) {
    return Operation{parseJobId(jobIdStr), parseOperationId(operationIdStr)};
}
