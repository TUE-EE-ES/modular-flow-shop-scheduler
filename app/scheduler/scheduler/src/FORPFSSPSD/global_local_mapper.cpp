//
// Created by jmarc on 17/6/2021.
//

#include "FORPFSSPSD/global_local_mapper.hpp"

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "FORPFSSPSD/module.hpp"

#include "pch/containers.hpp"

using namespace FORPFSSPSD;

BasicGlobalLocalMapper BasicGlobalLocalMapper::from(const ProductionLine &instance) {
    MOtO localToGlobalOps;
    OtMO globalToLocalOps;
    
    OperationId maxOpId = 0;
    for (const auto moduleId : instance.moduleIds()) {
        const auto &module = instance.getModule(moduleId);

        OperationId moduleOpIdOffset = maxOpId + 1;
        
        for (const auto &[jobId, ops] : module.jobs()) {
            for (const auto &op : ops) {
                const auto globalOp = operation{jobId, op.operationId + moduleOpIdOffset};
                
                globalToLocalOps.emplace(globalOp, std::make_tuple(moduleId, op));
                localToGlobalOps[moduleId].emplace(op, globalOp);
                
                maxOpId = std::max(maxOpId, op.operationId);
            }
        }
    }

    return {std::move(globalToLocalOps), std::move(localToGlobalOps)};
}

GlobalLocalMapper::GlobalLocalMapper(const Instance &problem,
                                     const std::map<ModuleId, ModuleInfo> &modulesDefinition) :
    m_p(problem) {
    mapGlobalToLocalFlowVector(modulesDefinition);
    mapGlobalToLocalOperations(modulesDefinition);
}

std::vector<std::tuple<operation, operation>>
GlobalLocalMapper::getGlobalAndLocalJobOps(const std::set<MachineId> &machinesSet,
                                           const JobId jobId) const {
    std::vector<std::tuple<operation, operation>> result;

    for (const auto &op : m_p.getJobOperations(jobId)) {
        // Check if the operation is done in this module
        if (machinesSet.find(m_p.getMachine(op)) == machinesSet.end()) {
            continue;
        }

        auto localOp = getLocalOpId(op.operationId);
        result.emplace_back(operation{jobId, op.operationId}, operation{jobId, localOp});
    }

    return result;
}

DefaultTimeBetweenOps &
GlobalLocalMapper::mapGlobalToLocalTimeFunc(
        const DefaultTimeBetweenOps &src,
        operation globalOp,
        DefaultTimeBetweenOps &dst) const {
    const auto it = src.find(globalOp);
    if (it == src.end()) {
        // If the operation does not have any times defined there's nothing to do.
        return dst;
    }

    const auto &[moduleId, localOp] = m_globalToLocalOps.at(globalOp);
    const auto &opTimes = it->second;
    for (const auto &[globalOpDst, delayValue] : opTimes) {
        // Find local mapping of the destination operation
        const auto &[opDstModuleId, localOpDst] = m_globalToLocalOps.at(globalOpDst);

        // Check if the destination operation belongs to the module
        if (moduleId != opDstModuleId) {
            continue;
        }
        dst.insert(localOp, localOpDst, delayValue);
    }

    return dst;
}

TimeBetweenOps &
GlobalLocalMapper::mapGlobalToLocalTimeFunc(
        const TimeBetweenOps &src,
        operation globalOp,
        TimeBetweenOps &dst) const {
    const auto it = src.find(globalOp);
    if (it == src.end()) {
        // If the operation does not have any times defined there's nothing to do.
        return dst;
    }

    const auto &[moduleId, localOp] = m_globalToLocalOps.at(globalOp);
    const auto &opTimes = it->second;
    for (const auto &[globalOpDst, delayValue] : opTimes) {
        // Find local mapping of the destination operation
        const auto &[opDstModuleId, localOpDst] = m_globalToLocalOps.at(globalOpDst);

        // Check if the destination operation belongs to the module
        if (moduleId != opDstModuleId) {
            continue;
        }
        dst.insert(localOp, localOpDst, delayValue);
    }

    return dst;
}

void GlobalLocalMapper::mapGlobalToLocalOperations(
        const std::map<ModuleId, ModuleInfo> &modulesDefinition) {
    for (const auto &[moduleId, module] : modulesDefinition) {
        for (const JobId jobId : m_p.getJobsOutput()) {
            for (auto &[globalOp, localOp] : getGlobalAndLocalJobOps(module.machines, jobId)) {
                // Add the operation mapping
                m_localToGlobalOps[moduleId].emplace(localOp, globalOp);
                m_globalToLocalOps.emplace(globalOp, std::make_tuple(module.id, localOp));
            }
        }
    }
}

void GlobalLocalMapper::mapGlobalToLocalFlowVector(
        const std::map<ModuleId, ModuleInfo> &modulesDefinition) {
    std::unordered_map<ModuleId, uint32_t> nextMachineId;
    std::unordered_map<MachineId, ModuleId> machineToModule;
    std::unordered_map<ModuleId, uint32_t> nextOperationId;
    std::unordered_set<MachineId> addedMachines;

    for (const auto &[moduleId, module] : modulesDefinition) {
        for (const auto &machine : module.machines) {
            machineToModule.emplace(machine, module.id);
        }
    }

    for (OperationId globalOpId : m_p.getOperationsFlowVector()) {
        MachineId machineId = m_p.getMachine(globalOpId);
        ModuleId moduleId = machineToModule.at(machineId);

        // Add local machine mapping use the added machines set to prevent giving different IDs to
        // re-entrant machines appearing more than once
        if (addedMachines.find(machineId) == addedMachines.end()) {
            auto localMachineId = static_cast<MachineId>(nextMachineId[moduleId]++);
            m_localToGlobalMachine.emplace(std::make_tuple(moduleId, localMachineId), machineId);
            m_globalToLocalMachine.emplace(machineId, std::make_tuple(moduleId, localMachineId));
            addedMachines.emplace(machineId);
        }

        // Add local operation mapping
        auto localOpId = static_cast<OperationId>(nextOperationId[moduleId]++);
        m_localToGlobalOpsId[moduleId].emplace(localOpId, globalOpId);
        m_globalToLocalOpsId.emplace(globalOpId, std::make_tuple(moduleId, localOpId));
    }
}
