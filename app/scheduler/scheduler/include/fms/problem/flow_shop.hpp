/*
 * flow_shop.hpp
 *
 *  Created on: 22 May 2014
 *      Author: uwaqas
 */

#ifndef FMS_PROBLEMS_FLOW_SHOP_HPP
#define FMS_PROBLEMS_FLOW_SHOP_HPP

#include "aliases.hpp"
#include "indices.hpp"
#include "maintenance_policy.hpp"
#include "operation.hpp"

#include "fms/cg/constraint_graph.hpp"
#include "fms/cli/command_line.hpp"
#include "fms/solvers/partial_solution.hpp"

#include <utility>

/**
 * @brief Namespace for the Fixed ORder Permutation FlowShop Scheduling Problem with
 * Sequence-Dependent setup time (FORPFSSPSD)
 */
namespace fms::problem {

/*
 * A FORPFSSPSD is a Fixed Order Permutation Flowshop
 * Sequence-dependent Setup-time Scheduling Problem
 *
 * It knows about the structure of precedence of operations on the machines,
 * and the jobs that need to be executed on those machines. The
 */
class Instance {
public:
    constexpr const static problem::JobId MAINT_ID{cg::ConstraintGraph::NEXT_ID - 3U};

    /**
     * @brief Constructs an instance of the problem
     * @param problemName The name of the problem
     * @param jobs The jobs to be scheduled
     * @param machineMapping The mapping of operations to machines
     * @param processingTimes The processing times for each operation
     * @param setupTimes The setup times between operations
     * @param setupTimesIndep The setup times between operations, independent of sequence
     * @param dueDates The due dates for each operation
     * @param dueDatesIndep The due dates for each operation, independent of sequence
     * @param absoluteDueDates The absolute due dates for each job
     * @param sheetSizes The sheet sizes for each operation
     * @param maximumSheetSize The maximum sheet size
     * @param shopType The type of shop (flow shop or job shop)
     * @param outOfOrder Whether the operations can be out of order
     */
    Instance(std::string problemName,
             JobOperations jobs,
             OperationMachineMap machineMapping,
             DefaultOperationsTime processingTimes,
             DefaultTimeBetweenOps setupTimes,
             TimeBetweenOps setupTimesIndep,
             TimeBetweenOps dueDates,
             TimeBetweenOps dueDatesIndep,
             JobsTime absoluteDueDates,
             OperationSizes sheetSizes,
             delay maximumSheetSize,
             cli::ShopType shopType = cli::ShopType::FIXEDORDERSHOP,
             bool outOfOrder = true);
    Instance(const Instance &other) = default;
    Instance(Instance &&other) noexcept = default;

    virtual ~Instance() = default;

    /**
     * @brief Copy assignment operator for the Instance class
     * @param other The Instance object to copy from
     * @return A reference to the current Instance object
     */
    Instance &operator=(const Instance &other) = default;

    /**
     * @brief Move assignment operator for the Instance class
     * @param other The Instance object to move from
     * @return A reference to the current Instance object
     */
    Instance &operator=(Instance &&other) = default;

    /**
     * @brief Get the jobs in the Instance
     * @return A reference to the JobOperations object in the Instance
     */
    [[nodiscard]] const JobOperations &jobs() const { return m_jobs; }

    /**
     * @brief Get the jobs for a specific JobId
     * @param jId The JobId to get the jobs for
     * @return A reference to the jobs for the specified JobId
     */
    [[nodiscard]] const auto &jobs(JobId jId) const { return m_jobs.at(jId); }

    /**
     * @brief Get the machine mapping in the Instance
     * @return A reference to the OperationMachineMap object in the Instance
     */
    [[nodiscard]] const OperationMachineMap &machineMapping() const { return m_machineMapping; }

    /**
     * @brief Get the processing times in the Instance
     * @return A reference to the DefaultOperationsTime object in the Instance
     */
    [[nodiscard]] const DefaultOperationsTime &processingTimes() const { return m_processingTimes; }

    /**
     * @brief Get the processing time for a specific operation
     * @param op The operation to get the processing time for
     * @return The processing time for the specified operation
     */
    [[nodiscard]] auto processingTimes(const Operation &op) const { return m_processingTimes(op); }

    /**
     * @brief Get the setup times in the Instance
     * @return A reference to the DefaultTimeBetweenOps object in the Instance
     */
    [[nodiscard]] const DefaultTimeBetweenOps &setupTimes() const { return m_setupTimes; }

    /**
     * @brief Get the setup time between two operations
     * @param opFrom The operation to start from
     * @param opTo The operation to end at
     * @return The setup time between the two specified operations
     */
    [[nodiscard]] auto setupTimes(const Operation &opFrom, const Operation &opTo) const {
        return m_setupTimes(opFrom, opTo);
    }

    /**
     * @brief Get the independent setup times in the Instance
     * @return A reference to the TimeBetweenOps object in the Instance
     */
    [[nodiscard]] const TimeBetweenOps &setupTimesIndep() const { return m_setupTimesIndep; }

    /**
     * @brief Get the independent setup time between two operations
     * @param opFrom The operation to start from
     * @param opTo The operation to end at
     * @return The independent setup time between the two specified operations
     */
    [[nodiscard]] auto setupTimesIndep(const Operation &opFrom, const Operation &opTo) const {
        return m_setupTimesIndep(opFrom, opTo);
    }

    /**
     * @brief Get the due dates in the Instance
     * @return A reference to the TimeBetweenOps object in the Instance
     */
    [[nodiscard]] const TimeBetweenOps &dueDates() const { return m_dueDates; }

    /**
     * @brief Get the due date between two operations
     * @param opFrom The operation to start from
     * @param opTo The operation to end at
     * @return The due date between the two specified operations
     */
    [[nodiscard]] auto dueDates(const Operation &opFrom, const Operation &opTo) const {
        return m_dueDates(opFrom, opTo);
    }

    /**
     * @brief Get the independent due dates in the Instance
     * @return A reference to the TimeBetweenOps object in the Instance
     */
    [[nodiscard]] const TimeBetweenOps &dueDatesIndep() const { return m_dueDatesIndep; }

    /**
     * @brief Get the independent due date between two operations
     * @param opFrom The operation to start from
     * @param opTo The operation to end at
     * @return The independent due date between the two specified operations
     */
    [[nodiscard]] auto dueDatesIndep(const Operation &opFrom, const Operation &opTo) const {
        return m_dueDatesIndep(opFrom, opTo);
    }

    /**
     * @brief Get the absolute due dates in the Instance
     * @return A reference to the JobsTime object in the Instance
     */
    [[nodiscard]] const JobsTime &absoluteDueDates() const { return m_absoluteDueDates; }

    /**
     * @brief Get the shop type in the Instance
     * @return The ShopType of the Instance
     */
    [[nodiscard]] cli::ShopType shopType() const { return m_shopType; }

    /**
     * @brief Get the sheet sizes in the Instance
     * @return A reference to the map of operations to sheet sizes in the Instance
     */
    [[nodiscard]] const auto &sheetSizes() const { return m_sheetSizes; }

    /**
     * @brief Get the maximum sheet size in the Instance
     * @return The maximum sheet size of the Instance
     */
    [[nodiscard]] delay maximumSheetSize() const { return m_maximumSheetSize; }

    /**
     * @brief Get the maintenance policy in the Instance
     * @return A reference to the MaintenancePolicy object in the Instance
     */
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

    /**
     * @brief Get the machine where an operation is mapped
     * @param op The operation
     * @return The machine where the operation is mapped
     */
    [[nodiscard]] inline MachineId getMachine(const Operation &op) const {
        return m_machineMapping.at(op);
    }

    /**
     * @brief Get the machine for a given operation ID
     * @param opId The operation ID
     * @return The machine for the given operation ID
     */
    [[nodiscard]] inline MachineId getMachine(const OperationId opId) const {
        return m_operationToMachine.at(opId);
    }

    /**
     * @brief Check if an operation is valid (i.e., it is mapped to a machine)
     * @param op The operation
     * @return True if the operation is valid, false otherwise
     */
    [[nodiscard]] inline bool isValid(const Operation &op) const {
        return m_machineMapping.contains(op);
    }

    /**
     * @brief Get the machine ID
     * @param m The machine ID
     * @return The machine ID
     */
    [[nodiscard]] inline static MachineId getMachine(MachineId m) { return m; }

    /**
     * @brief Get the total number of operations
     * @return The total number of operations
     */
    [[nodiscard]] inline std::size_t getTotalOps() const { return m_machineMapping.size(); }

    /**
     * @brief Get the order of a machine
     * @param m The machine
     * @return The order of the machine
     */
    template <typename T> [[nodiscard]] inline std::size_t getMachineOrder(T &&m) const {
        const auto machine = getMachine(std::forward<T>(m));
        return m_machineToIndex.at(machine);
    }

    /**
     * @brief Get the operations flow vector
     * @return The operations flow vector
     */
    [[nodiscard]] inline const OperationFlowVector &getOperationsFlowVector() const {
        return m_flowVector;
    }

    /**
     * @brief Get the problem name
     * @return The problem name
     */
    [[nodiscard]] virtual std::string getProblemName() const { return m_problemName; }

    /**
     * @brief Set the problem name
     * @param name The new problem name
     */
    void setProblemName(const std::string &name) { m_problemName = name; }

    /**
     * @brief Set the maintenance policy
     * @param args The arguments for the maintenance policy
     */
    template <typename... Args> void setMaintenancePolicy(Args &&...args) {
        m_maintPolicy = MaintenancePolicy(std::forward<Args>(args)...);
    }

    /**
     * @brief Get the number of jobs in the Instance
     * @return The number of jobs in the Instance
     */
    [[nodiscard]] inline unsigned int getNumberOfJobs() const { return m_jobs.size(); }

    /**
     * @brief Get the jobs output in the Instance
     * @return A reference to the jobs output in the Instance
     */
    [[nodiscard]] inline const auto &getJobsOutput() const { return m_jobsOutput; }

    /**
     * @brief Get the job at a specific output position
     * @param position The output position
     * @return The job at the specified output position
     */
    [[nodiscard]] inline auto getJobAtOutputPosition(std::size_t position) const {
        return m_jobsOutput.at(position);
    }

    /**
     * @brief Get the output position of a specific job
     * @param jobId The ID of the job
     * @return The output position of the specified job
     */
    [[nodiscard]] inline std::size_t getJobOutputPosition(JobId jobId) const {
        return m_jobToOutputPosition.at(jobId);
    }

    /**
     * @brief Get the machines in the Instance
     * @return A reference to the vector of machines in the Instance
     */
    [[nodiscard]] inline const std::vector<MachineId> &getMachines() const { return m_machines; }

    /**
     * @brief Get the number of machines in the Instance
     * @return The number of machines in the Instance
     */
    [[nodiscard]] inline std::size_t getNumberOfMachines() const { return m_machines.size(); }

    /**
     * @brief Get the maximum sheet size in the Instance
     * @return The maximum sheet size in the Instance
     */
    [[nodiscard]] inline unsigned int getMaximumSheetSize() const { return m_maximumSheetSize; }

    /**
     * @brief Get the re-entrancies of a specific job in a specific re-entrant machine.
     * @param jobId The ID of the job
     * @param reEntrancy The index of the re-entrant machine (known as re-entrant ID). The
     * re-entrant ID only counts re-entrant machines.
     * @return The number re-entrancies of the job
     */
    [[nodiscard]] ReEntrancies getReEntrancies(JobId jobId,
                                               ReEntrantId reEntrancy = ReEntrantId(0)) const;

    /**
     * @brief Overload of @ref getReEntrancies.
     * @details In this overload, the operation @p op is used to select the job and machine
     * whose re-entrancies are being checked. If no re-entrancies are performed in this machine,
     * this function returns `1` (meaning that it's not a re-entrant machine).
     * @param op Operation used to select the job and machine to check re-entrancies.
     * @return ReEntrancies Number of re-entrancies and `1` if not re-entrant.
     */
    [[nodiscard]] ReEntrancies getReEntrancies(const Operation &op) const;

    /**
     * @brief Get the maximum re-entrancies of a specific machine
     * @param machineId The ID of the machine
     * @return The maximum re-entrancies of the machine
     */
    [[nodiscard]] inline ReEntrancies getMachineMaxReEntrancies(MachineId machineId) const {
        return ReEntrancies(m_operationsMappedOnMachine.at(machineId).size());
    }

    /**
     * @brief Get the re-entrancy table
     * @details The re-entrancy table shows the number of re-entrancies of each job in each
     * re-entrant machine. The table is indexed by the job ID and the re-entrant ID.
     * @return A reference to the PlexityTable object in the Instance
     */
    [[nodiscard]] inline const PlexityTable &getReEntranciesTable() const { return m_jobPlexity; }

    /**
     * @brief Get the sheet size for a specific operation
     * @param op The operation
     * @return The sheet size for the specified operation
     */
    [[nodiscard]] inline auto getSheetSize(Operation op) const { return m_sheetSizes(op); }

    /**
     * @brief Get the unique sheet sizes in the Instance
     * @return A set of unique sheet sizes in the Instance
     */
    [[nodiscard]] inline std::unordered_set<unsigned int> getUniqueSheetSizes() const {
        return getUniqueSheetSizes(0);
    }

    /**
     * @brief Get the unique sheet sizes in the Instance starting from a specific job
     * @param startJob The job to start from
     * @return A set of unique sheet sizes in the Instance starting from the specified job
     */
    [[nodiscard]] std::unordered_set<unsigned int> getUniqueSheetSizes(std::size_t startJob) const;

    /**
     * @brief Get the number of operations per job in the Instance
     * @return The number of operations per job in the Instance
     */
    [[nodiscard]] inline OperationId getNumberOfOperationsPerJob() const {
        return m_flowVector.size();
    }

    /**
     * @brief Get the processing time for a specific operation
     * @param op The operation
     * @return The processing time for the specified operation
     */
    [[nodiscard]] inline delay getProcessingTime(Operation op) const {
        return m_processingTimes(op);
    }

    /**
     * @brief Get the processing time for a specific vertex ID
     * @param id The vertex ID
     * @return The processing time for the specified vertex ID
     */
    [[nodiscard]] inline delay getProcessingTime(cg::VertexId id) const {
        return m_processingTimes(m_dg->getVertex(id).operation);
    }

    /**
     * @brief Get the setup time between two operations
     * @param op1 The first operation
     * @param op2 The second operation
     * @return The setup time between the two specified operations
     */
    [[nodiscard]] delay getSetupTime(Operation op1, Operation op2) const;

    /**
     * @brief Query the delay between two vertices in the delay graph
     * @param vertex1 The first vertex
     * @param vertex2 The second vertex
     * @return The delay between the two specified vertices
     */
    [[nodiscard]] delay query(const cg::Vertex &vertex1, const cg::Vertex &vertex2) const;

    /**
     * @brief Queries sequence-dependent setup time between two operations. That imposes the
     *        constraint time(src) + query(src, dst) <= time(dst)
     * @param src Source operation
     * @param dst Destination operation
     * @return delay Minimum time between the start of @p src and the start of @p dst
     */
    [[nodiscard]] delay query(const Operation &src, const Operation &dst) const;

    /**
     * @brief Computes the due date between two operations @p src and @p dst . If the value is found
     * in m_extraDueDates , the minimum among the found values is taken. If no values are found
     * nullopt is returned meaning that there is no deadline.
     *
     * @param src Source operation
     * @param dst Destination operation
     * @return std::optional<delay> Value if deadline found and nullopt otherwise.
     */
    [[nodiscard]] std::optional<delay> queryDueDate(const Operation &src,
                                                    const Operation &dst) const;

    /**
     * @brief Get the delay graph
     * @return A reference to the delay graph
     */
    [[nodiscard]] const cg::ConstraintGraph &getDelayGraph() const { return *m_dg; }

    /**
     * @brief Update the delay graph with a new one
     * @param newGraph The new delay graph
     */
    inline void updateDelayGraph(const cg::ConstraintGraph &newGraph) { m_dg = newGraph; }

    /**
     * @brief Update the delay graph with a new one (move semantics)
     * @param newGraph The new delay graph
     */
    inline void updateDelayGraph(cg::ConstraintGraph &&newGraph) { m_dg = std::move(newGraph); }

    /**
     * @brief Check if the delay graph is initialized
     * @return True if the delay graph is initialized, false otherwise
     */
    [[nodiscard]] bool isGraphInitialized() const noexcept { return m_dg.has_value(); }

    /**
     * @brief Create the final sequence from a partial solution
     * @param ps The partial solution
     * @return The final sequence
     */
    // [[nodiscard]] cg::Edges createFinalSequence(const solvers::PartialSolution &ps) const;

    /**
     * @brief Get the ID of the re-entrant machine
     * @param reEntrantId The ID of the re-entrant machine
     * @return The ID of the re-entrant machine
     */
    [[nodiscard]] inline MachineId getReEntrantMachineId(ReEntrantId reEntrantId) const {
        return m_reEntrantMachines.at(static_cast<std::size_t>(reEntrantId.value));
    }

    /**
     * @brief Find the re-entrant ID of a machine
     * @param machineId The ID of the machine
     * @return The re-entrant ID of the machine
     */
    [[nodiscard]] inline ReEntrantId findMachineReEntrantId(MachineId machineId) const {
        return m_reEntrantMachineToId.at(machineId);
    }

    /**
     * @brief Find the re-entrant ID of a machine for a specific operation
     * @param op The operation
     * @return The re-entrant ID of the machine for the specified operation
     */
    [[nodiscard]] inline ReEntrantId findMachineReEntrantId(const Operation &op) const {
        return findMachineReEntrantId(getMachine(op));
    }

    [[nodiscard]] inline std::optional<ReEntrantId> getFirstReEntrantId() const {
        if (m_reEntrantMachines.empty()) {
            return std::nullopt;
        }
        return m_reEntrantMachineToId.at(m_reEntrantMachines.front());
    }

    /**
     * @brief Check if an operation is valid (i.e., it is mapped to a machine)
     * @param op The operation
     * @return True if the operation is valid, false otherwise
     */
    [[nodiscard]] inline bool containsOp(const Operation &op) const {
        return m_machineMapping.find(op) != m_machineMapping.end();
    }

    /**
     * @brief Check if an operation is re-entrant
     * @param op The operation
     * @return True if the operation is re-entrant, false otherwise
     */
    [[nodiscard]] inline bool isReEntrantOp(const Operation &op) const {
        return isReEntrantMachine(getMachine(op));
    }

    /**
     * @brief Check if a machine is re-entrant
     * @param machineId The ID of the machine
     * @return True if the machine is re-entrant, false otherwise
     */
    [[nodiscard]] inline bool isReEntrantMachine(const MachineId machineId) const {
        return m_reEntrantMachineToId.find(machineId) != m_reEntrantMachineToId.end();
    }

    /**
     * @brief Set the out of order status
     * @param outOfOrder The new out of order status
     */
    constexpr void setOutOfOrder(bool outOfOrder) noexcept { m_outOfOrder = outOfOrder; }

    /**
     * @brief Check if the Instance is out of order
     * @return True if the Instance is out of order, false otherwise
     */
    [[nodiscard]] constexpr bool isOutOfOrder() const noexcept { return m_outOfOrder; }

    /**
     * @brief Get the re-entrant machines in the Instance
     * @return A reference to the vector of re-entrant machines in the Instance
     */
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
    void addExtraSetupTime(Operation src, Operation dst, delay value);

    /**
     * @brief Adds a due date between @p src and @p dst . The internal delay graph is updated
     * accordingly with the maximum value among the existing due date defined in dueDates and
     * @p value .
     *
     * @param src Source operation
     * @param dst Destination operation
     * @param value Value of the due date (positive)
     */
    void addExtraDueDate(Operation src, Operation dst, delay value);

    /**
     * @brief Get the operations that a job does in a machine
     * @param jobId Job ID of the job that we want to check
     * @param machineId Machine ID of the machine that we want to check
     * @return OperationsVector Operations that the job performs in the machine in the order
     * that they are performed.
     */
    [[nodiscard]] OperationsVector getJobOperationsOnMachine(JobId jobId,
                                                             MachineId machineId) const;

    /**
     * @brief Creates unique maintenance operations for the problem
     * @details These operations are not added to the problem, but they are created to be used
     * in the maintenance. They are guaranteed to be unique.
     * @param maintId ID of the type of maintenance
     * @return Operation Created operation
     */
    [[nodiscard]] inline Operation addMaintenanceOperation(MaintType maintId) {
        return Operation{MAINT_ID, m_nextMaintenanceOpId++, maintId};
    }

private:
    void computeFlowVector();

    void computeJobsOutput();

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
    cli::ShopType m_shopType;

    /// true if the input operations can be out of order and false otherwise. E.g., in the case
    /// of mixed-plexity, some input operations can happen latter.
    bool m_outOfOrder;

    // Maintenance related
    OperationSizes m_sheetSizes;
    delay m_maximumSheetSize;
    MaintenancePolicy m_maintPolicy;

    /// Constraint-graph model of the current problem. It needs to be set by an external function.
    std::optional<cg::ConstraintGraph> m_dg;

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

    /**
     * @brief Stores the operations of each job and the machine where they are executed.
     * @details If multiple operations are performed in the same machine, the order of the
     * operations is preserved in the vector.
     */
    std::unordered_map<JobId, std::unordered_map<MachineId, OperationsVector>> m_jobToMachineOps;

    OperationId m_nextMaintenanceOpId = 0;
};
} // namespace fms::problem

#endif /* FMS_PROBLEMS_FLOW_SHOP_HPP */
