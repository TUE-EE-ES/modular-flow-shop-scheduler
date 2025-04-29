#ifndef DD_VERTEX_HPP
#define DD_VERTEX_HPP

#include "fms/algorithms/hash.hpp"
#include "fms/algorithms/longest_path.hpp"
#include "fms/cg/constraint_graph.hpp"
#include "fms/problem/flow_shop.hpp"
#include "fms/problem/indices.hpp"

#include <cstdint>
#include <memory>
#include <utility>

namespace fms::dd {

using MachineToVertex = std::unordered_map<problem::MachineId, cg::VertexId>;
using JobIdxToOpIdx = std::vector<std::size_t>;
using solvers::MachinesSequences;

using VertexId = std::uint64_t;

class Vertex {
public:
    /**
     * @brief Constructs a new Vertex object.
     * @param id The unique identifier of the vertex.
     * @param parentId The unique identifier of the parent vertex.
     * @param sequences The sequences of machines associated with the vertex
     * @param ASAPST The earliest start time of each path from source to this vertex.
     * @param ALAPST The latest start time of each path from source to this vertex.
     * @param jobsCompletion Mapping of job index to operation index representing job completions.
     * @param jobOrder Order of jobs associated with the vertex.
     * @param lastOperation Mapping of machine to vertex representing the last operation.
     * @param scheduledOps Vertices IDs of operations scheduled.
     * @param vertexDepth Depth of the vertex in the delay graph.
     * @param encounteredOps Vertices IDs of encountered operations.
     */
    Vertex(VertexId id,
           VertexId parentId,
           MachinesSequences sequences,
           algorithms::paths::PathTimes ASAPST,
           algorithms::paths::PathTimes ALAPST,
           JobIdxToOpIdx jobsCompletion,
           std::vector<problem::JobId> jobOrder,
           MachineToVertex lastOperation,
           cg::VerticesIds scheduledOps = {},
           std::uint64_t vertexDepth = 0,
           cg::VerticesIds encounteredOps = {}) :
        m_id(id),
        m_parentId(parentId),
        m_machinesSequences(std::move(sequences)),
        m_ASAPST(std::move(ASAPST)),
        m_ALAPST(std::move(ALAPST)),
        m_jobsCompletion(std::move(jobsCompletion)),
        m_jobOrder(std::move(jobOrder)),
        m_lastOperation(std::move(lastOperation)),
        m_scheduledOps(std::move(scheduledOps)),
        m_vertexDepth(vertexDepth),
        m_encounteredOps(std::move(encounteredOps)) {}

    Vertex(const Vertex &other) = default;

    Vertex(Vertex &&other) noexcept = default;

    ~Vertex() = default;

    Vertex &operator=(const Vertex &other) = default;

    Vertex &operator=(Vertex &&other) = default;

    /**
     * @brief Equality comparison operator for Vertex objects.
     * @details Two vertices are considered equal if their unique identifiers are equal.
     * @return True if the vertices are equal, false otherwise.
     */
    [[nodiscard]] inline bool operator==(const Vertex &rhs) const noexcept {
        return m_id == rhs.m_id;
    }

    [[nodiscard]] inline VertexId id() const noexcept { return m_id; }

    [[nodiscard]] inline VertexId parentId() const noexcept { return m_parentId; }

    [[nodiscard]] inline delay lowerBound() const noexcept { return m_ASAPST.back(); }

    [[nodiscard]] inline std::uint64_t vertexDepth() const noexcept { return m_vertexDepth; }

    [[nodiscard]] inline problem::JobOperations readyOps() const noexcept { return m_readyOps; }

    /**
     * @brief Retrieves the vector of immediately ready operations associated with the vertex.
     * @details This function returns the first ready operation of each job, making up for the fact
     * that permutation flow shops allow for all operations of a ready job to be scheduled at once.
     * This is necessary for dominance checks and other code segments which evaluate based on
     * immediately ready jobs.
     * @return The vector of immediately ready operations.
     */
    [[nodiscard]] inline problem::OperationsVector immediatelyReadyOps() const {
        // returns first ready op of each job
        // makes up for the fact that permutation flow shops allow for all ops of a ready job to be
        // scheduled at once since no overtaking is allowed but the dominance check (and some other
        // code segments) only evaluate based on immediately ready jobs (recall that operations of a
        // job still have precedence constraints between them)
        problem::OperationsVector allImmediateOps;
        allImmediateOps.reserve(m_readyOps.size());
        for (const auto &[jId, ops] : m_readyOps) {
            allImmediateOps.emplace_back(ops.front());
        }
        return allImmediateOps;
    }

    [[nodiscard]] inline cg::VerticesIds scheduledOps() const noexcept { return m_scheduledOps; }

    [[nodiscard]] inline cg::VerticesIds encounteredOps() const noexcept {
        return m_encounteredOps;
    }

    [[nodiscard]] inline const auto &getMachinesSequences() const noexcept { return m_machinesSequences; }
    inline void setMachinesSequences(MachinesSequences newMachinesSequences) {
        m_machinesSequences = std::move(newMachinesSequences);
    }

    /**
     * @brief Generates all the edges associated with the current sequences
     * @param g Constraint graph to which the edges belong
     * @return cg::Edges List of edges associated with the sequences
     */
    [[nodiscard]] cg::Edges getAllEdges(const problem::Instance &problem) const;

    /**
     * @brief Retrieves the As Late as Possible Start Time.
     * @return Vector associating @ref cg::VertexID to its ALAPST. This in turn can be used
     * to determine the latest start time of each operation.
     */
    [[nodiscard]] inline const auto &getALAPST() const noexcept { return m_ALAPST; }

    inline void setALAPST(algorithms::paths::PathTimes newALAPST) { m_ALAPST = std::move(newALAPST); }

    /**
     * @brief Retrieves the As Soon as Possible Start Time.
     * @return Vector associating @ref cg::VertexID to its ASAPST. This in turn can be used
     * to determine the earliest start time of each operation.
     */
    [[nodiscard]] inline const auto &getASAPST() const noexcept { return m_ASAPST; }

    /// @details This is an overloaded function. It differs from @ref getASAPST() in that it returns
    /// a mutable reference to the ASAPST.
    [[nodiscard]] inline auto getASAPST() noexcept { return m_ASAPST; }

    inline void setASAPST(algorithms::paths::PathTimes newASAPST) { m_ASAPST = std::move(newASAPST); }

    [[nodiscard]] inline const auto &getJobsCompletion() const noexcept { return m_jobsCompletion; }

    [[nodiscard]] inline auto &getJobsCompletion() noexcept { return m_jobsCompletion; }

    /// @brief Check whether the vertex is terminal.
    /// @details A vertex is considered terminal if all the operations have been scheduled.
    /// @return `true` if it's terminal, `false` otherwise.
    [[nodiscard]] inline bool getTerminal() const noexcept { return m_terminal; }

    inline void setTerminal(bool value) noexcept { m_terminal = value; }

    [[nodiscard]] inline const auto &getJobOrder() const noexcept { return m_jobOrder; }

    inline void setJobOrder(std::vector<problem::JobId> newJobOrder) {
        m_jobOrder = std::move(newJobOrder);
    }

    /**
     * @brief Get the last vertex of each machine
     */
    [[nodiscard]] inline const auto &getLastOperation() const noexcept { return m_lastOperation; }
    inline void setLastOperation(MachineToVertex lastOperation) {
        m_lastOperation = std::move(lastOperation);
    }

    inline void setReadyOperations(problem::JobOperations readyOps) {
        m_readyOps = std::move(readyOps);
    }

    inline void removeReadyOperation(problem::JobId id) { m_readyOps.extract(id); }

    void setReadyOperations(const problem::Instance &problem, bool graphIsRelaxed = false) {
        m_readyOps.clear();
        const auto &jobs = problem.jobs();
        const auto &jobsOutput = problem.getJobsOutput();

        // For all jobs find the possible operations that can be scheduled
        for (std::size_t i = 0; i < jobs.size(); ++i) {
            // jobsOutput contains all IDs of jobs. The order of jobsOutput only matters for the
            // fixed output order flow-shop
            const auto jobId = jobsOutput[i];
            const auto opIdx = m_jobsCompletion.at(i);
            const auto &jobOps = jobs.at(jobId); // no chosen permutation for any shop

            if (opIdx >= jobOps.size()) {
                continue;
            }
            // if fixed order flowshop with no overtaking, then previous job must have at least done
            // that operation index too
            if (problem.shopType() == cli::ShopType::FIXEDORDERSHOP && i > 0) {
                if (m_jobsCompletion.at(i - 1) <= opIdx) { // TODO: Mixed plexity breaks this
                    continue;
                }
            }

            // generalise no overtaking in flowshop
            // when we relax the graph, we allow overtaking because merge loses information on job
            // ordering
            //  and we do not want to exclude any solutions
            if (problem.shopType() == cli::ShopType::FLOWSHOP && opIdx > 0 && !graphIsRelaxed) {
                const auto &jobOrder = m_jobOrder;
                // this is a flowshop and an operation of this job has been scheduled at this state
                // and thus a relative order established
                auto pos = std::find(jobOrder.begin(), jobOrder.end(), jobId);
                if (pos != jobOrder.end() && std::distance(pos, jobOrder.begin()) > 0) {
                    const auto jobPrevPos = problem.getJobOutputPosition(*(pos - 1));
                    if (m_jobsCompletion.at(jobPrevPos) < opIdx) {
                        continue;
                    }
                }
            }
            // Add the operation to ready queue
            // LOG(FMT_COMPILE("Adding operation {} of job {} to the queue"), opIdx, i);
            // in a permutation flowshop, there is no overlap so all operations of a job can be
            // ready at once, it is based on job order
            m_readyOps[jobId] = problem.shopType() == cli::ShopType::FLOWSHOP
                                        ? jobOps
                                        : problem::OperationsVector{jobOps[opIdx]};
        }
        // LOG("End of ready queue at this time\n\n");
    }

    [[nodiscard]] inline std::optional<cg::VertexId> getLastScheduledOperation() const {
        if (m_scheduledOps.empty()) {
            return std::nullopt;
        }
        return m_scheduledOps.back();
    }

private:
    VertexId m_id;

    VertexId m_parentId;

    /// Sequences of operations per machine
    MachinesSequences m_machinesSequences;

    /// Current known earliest start times
    algorithms::paths::PathTimes m_ASAPST;

    /// Current known latest start times
    algorithms::paths::PathTimes m_ALAPST;

    /// Index of the next operation to do for each job. The index is not an OperationId but an index
    /// in the vector of operations of the job.
    JobIdxToOpIdx m_jobsCompletion;

    // True if state is a terminal state i.e. all operations of all jobs have been scheduled
    bool m_terminal = false;

    /// Index of the job ordering, used in state expansion when no overtaking is allowed
    /// Job order inferred and filled from the relationship between initial operations of jobs in
    /// that state Immaterial for job shops unless no overtaking specified (case currently not
    /// considered), important for flowshops to obey no overtaking
    std::vector<problem::JobId> m_jobOrder;

    /// Next ready operations from this state (feasible set)
    problem::JobOperations m_readyOps;

    /// Operations already scheduled in this state
    cg::VerticesIds m_scheduledOps;

    /// Last operation on each machine
    MachineToVertex m_lastOperation;

    /// @brief Operations already scheduled in this state
    /// @details In the full decision diagram, they are exactly equal to m_scheduledOps. It takes on
    /// meaning in the relaxed decision diagram when we merge. It is the union of scheduled ops of
    /// all states that were merged to create new state
    cg::VerticesIds m_encounteredOps;

    /// Vertex depth to use for node selection
    std::uint64_t m_vertexDepth;
};

using SharedVertex = std::shared_ptr<dd::Vertex>;

} // namespace fms::dd

template <> struct std::hash<fms::dd::JobIdxToOpIdx> {
    std::size_t operator()(const fms::dd::JobIdxToOpIdx &k) const {
        std::size_t result = 0;
        for (const auto &i : k) {
            fms::algorithms::hash_combine(result, i);
        }
        return result;
    }
};

#endif // DD_VERTEX_HPP