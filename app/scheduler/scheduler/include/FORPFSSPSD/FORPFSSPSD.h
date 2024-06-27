/*
 * FORPFSSPSD.h
 *
 *  Created on: 22 May 2014
 *      Author: uwaqas
 */

#ifndef FORPFSSPSD_H_
#define FORPFSSPSD_H_

#include "FORPFSSPSD/aliases.hpp"
#include "FORPFSSPSD/indices.hpp"
#include "FORPFSSPSD/maintenance_policy.hpp"
#include "FORPFSSPSD/operation.h"
#include "FORPFSSPSD/plexity.hpp"
#include "delayGraph/delayGraph.h"
#include "partialsolution.h"
#include "utils/commandLine.h"

#include <utility>

/**
 * @brief Namespace for the Fixed ORder Permutation FlowShop Scheduling Problem with 
 * Sequence-Dependent setup time (FORPFSSPSD)
 */
namespace FORPFSSPSD {

/*
 * A FORPFSSPSD is a Fixed Order Permutation Flowshop
 * Sequence-dependent Setup-time Scheduling Problem
 *
 * It knows about the structure of precedence of operations on the machines,
 * and the jobs that need to be executed on those machines. The
 */
class Instance {

public:
    Instance(std::string problemName,
             JobOperations jobs,
             OperationMachineMap machineMapping,
             DefaultOperationsTime processingTimes,
             DefaultTimeBetweenOps setupTimes,
             TimeBetweenOps setupTimesIndep,
             TimeBetweenOps dueDates,
             TimeBetweenOps dueDatesIndep,
             JobsTime absoluteDueDates,
             std::map<operation, unsigned int> sheetSizes,
             delay defaultSheetSize,
             delay maximumSheetSize,
             ShopType shopType = ShopType::FIXEDORDERSHOP,
             bool outOfOrder = true);
    Instance(const Instance &other) = default;
    Instance(Instance &&other) noexcept = default;

    virtual ~Instance() = default;

    Instance &operator=(const Instance &other) = default;
    Instance &operator=(Instance &&other) = default;

    [[nodiscard]] const JobOperations &jobs() const { return m_jobs; }

    [[nodiscard]] const auto &jobs(JobId jId) const { return m_jobs.at(jId); }

    [[nodiscard]] const OperationMachineMap &machineMapping() const { return m_machineMapping; }

    [[nodiscard]] const DefaultOperationsTime &processingTimes() const { return m_processingTimes; }

    [[nodiscard]] auto processingTimes(const operation &op) const { return m_processingTimes(op); }

    [[nodiscard]] const DefaultTimeBetweenOps &setupTimes() const { return m_setupTimes; }

    [[nodiscard]] auto setupTimes(const operation &opFrom, const operation &opTo) const {
        return m_setupTimes(opFrom, opTo);
    }

    [[nodiscard]] const TimeBetweenOps &setupTimesIndep() const { return m_setupTimesIndep; }

    [[nodiscard]] auto setupTimesIndep(const operation &opFrom, const operation &opTo) const {
        return m_setupTimesIndep(opFrom, opTo);
    }

    [[nodiscard]] const TimeBetweenOps &dueDates() const { return m_dueDates; }

    [[nodiscard]] auto dueDates(const operation &opFrom, const operation &opTo) const {
        return m_dueDates(opFrom, opTo);
    }

    [[nodiscard]] const TimeBetweenOps &dueDatesIndep() const { return m_dueDatesIndep; }

    [[nodiscard]] auto dueDatesIndep(const operation &opFrom, const operation &opTo) const {
        return m_dueDatesIndep(opFrom, opTo);
    }

    [[nodiscard]] const JobsTime &absoluteDueDates() const { return m_absoluteDueDates; }

    [[nodiscard]] ShopType shopType() const { return m_shopType; }

    [[nodiscard]] const std::map<operation, unsigned int> &sheetSizes() const {
        return m_sheetSizes;
    }

    [[nodiscard]] delay defaultSheetSize() const { return m_defaultSheetSize; }

    [[nodiscard]] delay maximumSheetSize() const { return m_maximumSheetSize; }

    [[nodiscard]] const MaintenancePolicy &maintenancePolicy() const { return m_maintPolicy; }

    /**
     * @brief Get the mapping of each operation to the machine where it should be processed
     *
     * @return const OperationMachineMap& Dictionary mapping each operation to a machine
     */
    [[nodiscard]] inline const auto &getOperationsMappedOnMachine() const {
        return m_operationsMappedOnMachine;
    }

    /**
     * @brief Return the operations of the flow vector that are mapped into the machine. If more
     * than one operation is mapped then it is a re-entrant machine.
     * 
     * @param machineId Id of the machine to find its mapped operations.
     * @return const auto& Operations from the flow vector mapped into the machine.
     */
    [[nodiscard]] inline const auto &getMachineOperations(const MachineId machineId) const {
        return m_operationsMappedOnMachine.at(machineId);
    }

    [[nodiscard]] inline MachineId getMachine(const operation &op) const {
        return m_machineMapping.at(op);
    }

    [[nodiscard]] inline MachineId getMachine(const OperationId opId) const {
        return m_operationToMachine.at(opId);
    }

    [[nodiscard]] inline bool isValid(const operation &op) const {
        return m_machineMapping.contains(op);
    }

    [[nodiscard]] inline static MachineId getMachine(MachineId m) { return m; }

    [[nodiscard]] inline std::size_t getTotalOps() const { return m_machineMapping.size(); }

    template <typename T> [[nodiscard]] inline std::size_t getMachineOrder(T &&m) const {
        const auto machine = getMachine(std::forward<T>(m));
        return m_machineToIndex.at(machine);
    }

    [[nodiscard]] inline const OperationFlowVector &getOperationsFlowVector() const {
        return m_flowVector;
    }

    [[nodiscard]] virtual std::string getProblemName() const { return m_problemName; }

    void setProblemName(const std::string &name) { m_problemName = name; }

    template <typename... Args> void setMaintenancePolicy(Args &&...args) {
        m_maintPolicy = MaintenancePolicy(std::forward<Args>(args)...);
    }

    [[nodiscard]] inline unsigned int getNumberOfJobs() const { return m_jobs.size(); }

    [[nodiscard]] inline const auto &getJobsOutput() const { return m_jobsOutput; }

    [[nodiscard]] inline auto getJobAtOutputPosition(std::size_t position) const {
        return m_jobsOutput.at(position);
    }

    [[nodiscard]] inline std::size_t getJobOutputPosition(JobId jobId) const {
        return m_jobToOutputPosition.at(jobId);
    }

    [[nodiscard]] inline const std::vector<MachineId> &getMachines() const { return m_machines; }

    [[nodiscard]] inline std::size_t getNumberOfMachines() const { return m_machines.size(); }
    [[nodiscard]] inline unsigned int getMaximumSheetSize() const { return m_maximumSheetSize; }

    /**
     * @brief Get the plexity of a specific job
     * @param jobId ID of job
     * @param reentrancy Index of the reentrant machine to obtain the plexity. It only counts
     *        reentrant machines so the first reentrant machine will have `reentrancy = 0`
     * @return Plexity of the job with ID @p jobId on the machine with reentrancy @p reentrancy.
     */
    [[nodiscard]] Plexity getPlexity(JobId jobId, ReEntrantId reentrancy = ReEntrantId(0)) const;

    [[nodiscard]] inline Plexity getPlexity(const operation &op) const {
        return getPlexity(op.jobId);
    }

    [[nodiscard]] ReEntrancies getJobReEntrancies(JobId jobId,
                                                  ReEntrantId reentrancy = ReEntrantId(0)) const;

    [[nodiscard]] inline ReEntrancies getMachineMaxReEntrancies(MachineId machineId) const {
        return ReEntrancies(m_operationsMappedOnMachine.at(machineId).size());
    }

    [[nodiscard]] inline const PlexityTable &getPlexityTable() const { return m_jobPlexity; }

    /**
     * @brief Get the number of re-entrancies of a job in a specific machine.
     * @details The operation @p op is used to select the job and machine whose re-entrancies are
     * being checked. If no re-entrancies are performed in this machine, this function returns `1`.
     * @param op Operation used to select the job and machine to check re-entrancies.
     * @return ReEntrancies Number of re-entrancies and `1` if not re-entrant.
     */
    [[nodiscard]] ReEntrancies getReEntrancies(const operation &op) const;

    [[nodiscard]] delay getSheetSize(operation op) const;

    [[nodiscard]] inline std::unordered_set<unsigned int> getUniqueSheetSizes() const {
        return getUniqueSheetSizes(0);
    }

    [[nodiscard]] std::unordered_set<unsigned int> getUniqueSheetSizes(std::size_t startJob) const;

    [[nodiscard]] inline OperationId getNumberOfOperationsPerJob() const {
        return m_flowVector.size();
    }

    void save(const PartialSolution &best, const commandLineArgs &) const;
    void save(DelayGraph::delayGraph &graph,
               std::string_view outputFile,
               ScheduleOutputFormat outputFormat,
               AlgorithmType algorithm) const;
    [[nodiscard]] nlohmann::json saveJSON(const PartialSolution &solution) const;

    [[nodiscard]] inline delay getProcessingTime(operation op) const {
        return m_processingTimes(op);
    }
    [[nodiscard]] inline delay getProcessingTime(DelayGraph::VertexID id) const {
        return m_processingTimes(m_dg->get_vertex(id).operation);
    }

    [[nodiscard]] delay getSetupTime(operation op1, operation op2) const;

    delay getTrivialCompletionTimeLowerbound();

    [[nodiscard]] delay query(const DelayGraph::vertex &vertex1,
                              const DelayGraph::vertex &vertex2) const;

    /**
     * @brief Queries sequence-dependent setup time between two operations. That imposes the
     *        constraint time(src) + query(src, dst) <= time(dst)
     * @param src Source operation
     * @param dst Destination operation
     * @return delay Minimum time between the start of @p src and the start of @p dst
     */
    [[nodiscard]] delay query(const operation &src, const operation &dst) const;

    /**
     * @brief Computes the due date between two operations @p src and @p dst . If the value is found
     * in m_extraDueDates , the minimum among the found values is taken. If no values are found
     * nullopt is returned meaning that there is no deadline.
     *
     * @param src Source operation
     * @param dst Destination operation
     * @return std::optional<delay> Value if deadline found and nullopt otherwise.
     */
    [[nodiscard]] std::optional<delay> queryDueDate(const operation &src,
                                                    const operation &dst) const;

    [[nodiscard]]  const DelayGraph::delayGraph &getDelayGraph() const {
        return *m_dg;
    }

    inline void updateDelayGraph(const DelayGraph::delayGraph &newGraph) {
        m_dg = newGraph;
    }

    inline void updateDelayGraph(DelayGraph::delayGraph &&newGraph) { m_dg = std::move(newGraph); }

    [[nodiscard]] bool isGraphInitialized() const noexcept {
        return m_dg.has_value();
    }

    [[nodiscard]] DelayGraph::Edges
    infer_pim_edges(const PartialSolution &ps) const;

    [[nodiscard]] DelayGraph::Edges
    createFinalSequence(const PartialSolution &ps) const;

    // determine a (partial) solution from provided operation times
    [[nodiscard]] PartialSolution determine_partial_solution(std::vector<delay> ASAPST) const;

    [[nodiscard]] PartialSolution loadSequence(std::istream &stream) const;

    /**
     * @brief Returns the operations of every job in the order that they should be processed
     * 
     * @param jobId Job whose operations are to be returned
     * @return const std::vector<operation>& Ordered vector of operations
     */
    [[nodiscard]] inline const std::vector<operation> &getJobOperations(JobId jobId) const {
        return m_jobs.at(jobId);
    }

    [[nodiscard]] inline const JobOperations &getJobOperations() const { return m_jobs; }

    [[nodiscard]] inline MachineId getReEntrantMachineId(ReEntrantId reEntrantId) const {
        return m_reEntrantMachines.at(static_cast<std::size_t>(reEntrantId.value));
    }

    [[nodiscard]] inline ReEntrantId findMachineReEntrantId(MachineId machineId) const {
        return m_reEntrantMachineToId.at(machineId);
    }

    [[nodiscard]] inline ReEntrantId findMachineReEntrantId(const operation &op) const {
        return findMachineReEntrantId(getMachine(op));
    }

    [[nodiscard]] inline bool containsOp(const operation &op) const {
        return m_machineMapping.find(op) != m_machineMapping.end();
    }

    [[nodiscard]] inline bool isReEntrantOp(const operation &op) const {
        return isReEntrantMachine(getMachine(op));
    }

    [[nodiscard]] inline bool isReEntrantMachine(const MachineId machineId) const {
        return m_reEntrantMachineToId.find(machineId) != m_reEntrantMachineToId.end();
    }

    constexpr void setOutOfOrder(bool outOfOrder) noexcept { m_outOfOrder = outOfOrder; }

    [[nodiscard]] constexpr bool isOutOfOrder() const noexcept { return m_outOfOrder; }

    [[nodiscard]] constexpr const std::vector<MachineId> &getReEntrantMachines() const {
        return m_reEntrantMachines;
    }

    /**
     * @brief Adds a setup time between @p src and @p dst . The internal delay graph is updated
     * accordingly with the maximum value among the existing setup time defined in setupTimes and
     * @p value . If there was already an existing extra setup time, the maximum value is taken.
     *
     * @param src Source operation
     * @param dst Destination operation
     * @param value Value of the setup time
     */
    void addExtraSetupTime(operation src, operation dst, delay value);

    /**
     * @brief Adds a due date between @p src and @p dst . The internal delay graph is updated
     * accordingly with the maximum value among the existing due date defined in dueDates and
     * @p value .
     *
     * @param src Source operation
     * @param dst Destination operation
     * @param value Value of the due date (positive)
     */
    void addExtraDueDate(operation src, operation dst, delay value);

private:
    /// Flow of operations of every job
    JobOperations m_jobs;

    /// Maps an operation to its machine
    OperationMachineMap m_machineMapping;

    /// Processing time of each operation
    DefaultOperationsTime m_processingTimes;

    /// Sequence-dependent setup time between two operations
    DefaultTimeBetweenOps m_setupTimes;

    /// Sequence-independent setup time between two operations
    TimeBetweenOps m_setupTimesIndep;

    /// Maximum time between the start time of two sequence-dependent operations (due date)
    TimeBetweenOps m_dueDates;

    /// Maximum time between the start time of two sequence-independent operations (due date)
    TimeBetweenOps m_dueDatesIndep;

    /// Maximum start time of a job (if defined)
    JobsTime m_absoluteDueDates;

    /// Type of the scheduling problem. It can be flow-shop or job-shop.
    ShopType m_shopType;

    /// true if the input operations can be out of order and false otherwise.
    bool m_outOfOrder;

    // Maintenance related
    std::map<operation, unsigned int> m_sheetSizes;
    delay m_defaultSheetSize;
    delay m_maximumSheetSize;
    MaintenancePolicy m_maintPolicy;

    /// Constraint-graph model of the current problem. It needs to be set by an external function.
    std::optional<DelayGraph::delayGraph> m_dg;

    /// @brief Vector of jobs in the system
    /// @details The order of the jobs is only relevant for fixed-output-order flow shops where
    /// it indicates the output order of the jobs.
    std::vector<JobId> m_jobsOutput;

    /// @brief Maps each job to its output position
    /// @details The index can be used in @ref m_jobsOutput to obtain the job in the output order
    std::unordered_map<JobId, std::size_t> m_jobToOutputPosition;

    /// Vector of operations in order that they should be processed. Only valid for flow shops
    OperationFlowVector m_flowVector;

    /// Maps each machine to the operations of the flow vector
    MachineMapOperationFlowVector m_operationsMappedOnMachine;

    delay cachedTrivialLowerbound = std::numeric_limits<delay>::max();

    /// Setup times added dynamically during execution
    TimeBetweenOps m_extraSetupTimes;

    /// Deadlines added dynamically during execution
    TimeBetweenOps m_extraDueDates;

    /// Contains the indices of the re-entrant machines in order that they appear in the flow vector
    std::vector<MachineId> m_reEntrantMachines;

    /// Contains the machines in order in which they appear in the flow vector
    std::vector<MachineId> m_machines;

    /// Mapping between a machine and its order
    std::unordered_map<MachineId, std::size_t> m_machineToIndex;

    /// Useful for a system with multiple re-entrant machines. Relates the re-entrant machine to
    /// its index in m_reEntrantMachines. Useful to find the plexity of a job
    std::unordered_map<MachineId, ReEntrantId> m_reEntrantMachineToId;

    std::string m_problemName;

    PlexityTable m_jobPlexity;

    std::unordered_map<OperationId, MachineId> m_operationToMachine;

    void computeFlowVector();

    void computeJobsOutput();
};
} // namespace FORPFSSPSD

#endif /* FORPFSSPSD_H_ */
