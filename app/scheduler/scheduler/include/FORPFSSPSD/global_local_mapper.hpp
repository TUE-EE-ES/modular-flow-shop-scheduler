//
// Created by jmarc on 17/6/2021.
//

#ifndef GLOBAL_LOCAL_MAPPER_HPP
#define GLOBAL_LOCAL_MAPPER_HPP

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "FORPFSSPSD/module.hpp"
#include "indices.hpp"
#include "production_line.hpp"

#include "pch/containers.hpp"

// Hash specialization to be able to use the tuple as a hash key
template <> struct std::hash<std::tuple<FORPFSSPSD::ModuleId, FORPFSSPSD::MachineId>> {
    std::size_t operator()(const std::tuple<FORPFSSPSD::ModuleId, FORPFSSPSD::MachineId> &t) const {
        std::size_t result = std::hash<FORPFSSPSD::ModuleId>()(std::get<0>(t));
        algorithm::hash_combine(result, std::get<1>(t));
        return result;
    }
};

namespace FORPFSSPSD {
class Instance;
class ModuleInfo;

class BasicGlobalLocalMapper {
public:
    using OtMO = std::unordered_map<operation, std::tuple<ModuleId, operation>>;
    using MOtO = std::unordered_map<ModuleId, std::unordered_map<operation, operation>>;

    BasicGlobalLocalMapper(OtMO globalToLocalOps, MOtO localToGlobalOps) :
        m_globalToLocalOps(std::move(globalToLocalOps)),
        m_localToGlobalOps(std::move(localToGlobalOps)) {}

    [[nodiscard]] inline const operation &getGlobalOp(ModuleId moduleId, operation localOp) const {
        return m_localToGlobalOps.at(moduleId).at(localOp);
    }

    [[nodiscard]] inline const std::tuple<ModuleId, operation> &
    getLocalOp(operation globalOp) const {
        return m_globalToLocalOps.at(globalOp);
    }

    static BasicGlobalLocalMapper from(const ProductionLine &instance);

private:
    std::unordered_map<operation, std::tuple<ModuleId, operation>> m_globalToLocalOps;
    std::unordered_map<ModuleId, std::unordered_map<operation, operation>> m_localToGlobalOps;
};

class GlobalLocalMapper {
public:
    GlobalLocalMapper(const Instance &problem,
                      const std::map<ModuleId, ModuleInfo> &modulesDefinition);

    /**
     * @brief Returns the operations of a job that are performed in a specific set of machines using
     *        both global and local indices
     * @param machinesSet Set of machines used to define the new set of operations.
     * @param jobId ID of the job
     * @return Vector with all the operations that job with `jobId` performs in the set of machines
     *         @p machinesSet. The first value of the pair is the global index and the second
     *         value is the local index.
     */
    [[nodiscard]] std::vector<std::tuple<operation, operation>>
    getGlobalAndLocalJobOps(const std::set<MachineId> &machinesSet, JobId jobId) const;

    [[nodiscard]] inline const operation &getGlobalOp(ModuleId moduleId, operation localOp) const {
        return m_localToGlobalOps.at(moduleId).at(localOp);
    }

    [[nodiscard]] inline const std::tuple<ModuleId, operation> &
    getLocalOp(operation globalOp) const {
        return m_globalToLocalOps.at(globalOp);
    }

    [[nodiscard]] inline OperationId getGlobalOpId(ModuleId moduleId, OperationId localOpId) const {
        return m_localToGlobalOpsId.at(moduleId).at(localOpId);
    }

    [[nodiscard]] inline OperationId getLocalOpId(OperationId globalOpId) const {
        return std::get<1>(m_globalToLocalOpsId.at(globalOpId));
    }

    [[nodiscard]] inline const auto &getModuleLocalToGlobalOps(ModuleId moduleId) const {
        return m_localToGlobalOps.at(moduleId);
    }

    [[nodiscard]] inline const MachineId &getGlobalMachine(ModuleId moduleId,
                                                           MachineId localMachineId) const {
        return m_localToGlobalMachine.at(std::make_tuple(moduleId, localMachineId));
    }

    [[nodiscard]] inline MachineId getLocalMachine(MachineId globalMachineId) const {
        return std::get<1>(m_globalToLocalMachine.at(globalMachineId));
    }

    [[nodiscard]] inline ModuleId getModuleIdFromMachine(MachineId globalMachineId) const {
        return std::get<0>(m_globalToLocalMachine.at(globalMachineId));
    }

    [[nodiscard]] inline ModuleId getModuleIdFromOpId(OperationId globalOpId) const {
        return std::get<0>(m_globalToLocalOpsId.at(globalOpId));
    }

    /**
     * @brief Translates the operation indices of the map @p src and saves them in the map @p dst
     *        for a single source operation with global index @p globalOp. Note that the
     *        destination operations will only be added to @p dst if they belong to the same module
     *        as the operation with index @p globalOp.
     *
     *        Note that if @p src is representing the function @f$F(o_1, o_2) = v@f$, this method
     *        only copies the values where the passed operation is representing @f$o_1@f$.
     * @param[in] src Source map containing the times for operations with global indexing.
     * @param[in] globalOp Global index of the operation that all its times are going to be copied.
     * @param[out] dst Destination map where the times are saved for operations with local indexing.
     */
    FORPFSSPSD::DefaultTimeBetweenOps &
    mapGlobalToLocalTimeFunc(const FORPFSSPSD::DefaultTimeBetweenOps &src,
                             operation globalOp,
                             FORPFSSPSD::DefaultTimeBetweenOps &dst) const;

    FORPFSSPSD::TimeBetweenOps &mapGlobalToLocalTimeFunc(const FORPFSSPSD::TimeBetweenOps &src,
                                                          operation globalOp,
                                                          FORPFSSPSD::TimeBetweenOps &dst) const;

private:
    const Instance &m_p;

    /// Relates an operation from the local problem to the global problem.
    std::unordered_map<ModuleId, std::unordered_map<operation, operation>> m_localToGlobalOps;

    /// Relates an operation from the global problem to the local problem
    std::unordered_map<operation, std::tuple<ModuleId, operation>> m_globalToLocalOps;

    /// Relates an operation ID from the local problem to the global problem.
    std::unordered_map<ModuleId, std::unordered_map<OperationId, OperationId>> m_localToGlobalOpsId;

    /// Relates an operation ID from the global problem to the local problem.
    std::unordered_map<OperationId, std::tuple<ModuleId, OperationId>> m_globalToLocalOpsId;

    /// Relates a machine from the local problem to the global problem
    std::unordered_map<std::tuple<ModuleId, MachineId>, MachineId> m_localToGlobalMachine;

    /// Relates a global machine ID to the module and local machine ID
    std::unordered_map<MachineId, std::tuple<ModuleId, MachineId>> m_globalToLocalMachine;

    /**
     * @brief Creates the mapping from all global operations to local operations and modules.
     *
     *        This mapping is needed because the FORPFSSPS::Instance class assumes that the IDs of
     *        operations always start at 0
     * @param modulesDefinition Definition of modules
     */
    void mapGlobalToLocalOperations(const std::map<ModuleId, ModuleInfo> &modulesDefinition);

    /**
     * @brief Creates the mapping from global to local machines, modules and flow vectors.
     *
     *        This mapping is needed because some part of the code of the FORPFSSPSD::Instance
     *        class and other methods that use it, assume that machine IDs start at 0.
     * @param modulesDefinition Definition of modules
     */
    void mapGlobalToLocalFlowVector(const std::map<ModuleId, ModuleInfo> &modulesDefinition);
};

} // namespace FORPFSSPSD

#endif // GLOBAL_LOCAL_MAPPER_HPP
