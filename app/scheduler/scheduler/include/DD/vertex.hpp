#ifndef DD_VERTEX_HPP
#define DD_VERTEX_HPP

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "FORPFSSPSD/indices.hpp"
#include "FORPFSSPSD/operation.h"
#include "delayGraph/delayGraph.h"
#include "delayGraph/edge.h"
#include "longest_path.h"

#include "pch/containers.hpp"
#include "pch/utils.hpp"
#include <cstdint>
#include <utility>

namespace DD {

using MachineToVertex = std::unordered_map<FORPFSSPSD::MachineId, DelayGraph::VertexID>;
using JobIdxToOpIdx = std::vector<std::size_t>;
using MachineEdges = std::unordered_map<FORPFSSPSD::MachineId, DelayGraph::Edges>;
// using VertexIdToVertex = std::unordered_map<std::uint64_t,std::shared_ptr<SeedVertex>>;

class Vertex {
public:
    Vertex(const Vertex &other) = default;

    Vertex(Vertex &&other) noexcept = default;

    Vertex(std::uint64_t id,
           MachineEdges edges,
           algorithm::PathTimes ASAPST,
           algorithm::PathTimes ALAPST,
           JobIdxToOpIdx jobsCompletion,
           std::vector<FORPFSSPSD::JobId> jobOrder,
           MachineToVertex lastOperation,
           DelayGraph::VerticesIDs scheduledOps = {},
           std::uint64_t vertexDepth = 0,
           DelayGraph::VerticesIDs encounteredOps = {}) :
        m_id(id),
        m_machineEdges(std::move(edges)),
        m_ASAPST(std::move(ASAPST)),
        m_ALAPST(std::move(ALAPST)),
        m_jobsCompletion(std::move(jobsCompletion)),
        m_jobOrder(std::move(jobOrder)),
        m_lastOperation(std::move(lastOperation)),
        m_scheduledOps(std::move(scheduledOps)),
        m_vertexDepth(vertexDepth),
        m_encounteredOps(std::move(encounteredOps)) {}

    ~Vertex() = default;

    Vertex &operator=(const Vertex &other) = default;

    Vertex &operator=(Vertex &&other) = default;

    [[nodiscard]] inline bool operator==(const Vertex &rhs) const noexcept {
        return m_id == rhs.m_id;
    }

    [[nodiscard]] inline std::uint64_t id() const noexcept { return m_id; }

    [[nodiscard]] inline delay lowerBound() const noexcept { return m_ASAPST.back(); }

    [[nodiscard]] inline std::uint64_t vertexDepth() const noexcept { return m_vertexDepth; }

    [[nodiscard]] inline FORPFSSPSD::JobOperations readyOps() const noexcept { return m_readyOps; }

    [[nodiscard]] inline FORPFSSPSD::OperationsVector immediatelyReadyOps() const {
        // returns first ready op of each job
        // makes up for the fact that permutation flow shops allow for all ops of a ready job to be
        // scheduled at once since no overtaking is allowed but the dominance check (and some other
        // code segments) only evaluate based on immediately ready jobs (recall that operations of a
        // job still have precedence constraints between them)
        FORPFSSPSD::OperationsVector allImmediateOps;
        allImmediateOps.reserve(m_readyOps.size());
        for (const auto &[jId, ops] : m_readyOps) {
            allImmediateOps.emplace_back(ops.front());
        }
        return allImmediateOps;
    }

    [[nodiscard]] inline DelayGraph::VerticesIDs scheduledOps() const noexcept {
        return m_scheduledOps;
    }
    [[nodiscard]] inline DelayGraph::VerticesIDs encounteredOps() const noexcept {
        return m_encounteredOps;
    }

    [[nodiscard]] inline const auto &getMachineEdges() const noexcept { return m_machineEdges; }
    inline void setMachineEdges(MachineEdges newMachineEdges) {
        m_machineEdges = std::move(newMachineEdges);
    }

    [[nodiscard]] DelayGraph::Edges getAllEdges() const;

    [[nodiscard]] inline const auto &getALAPST() const noexcept { return m_ALAPST; }
    inline void setALAPST(algorithm::PathTimes newALAPST) { m_ALAPST = std::move(newALAPST); }

    [[nodiscard]] inline const auto &getASAPST() const noexcept { return m_ASAPST; }
    [[nodiscard]] inline auto getASAPST() noexcept { return m_ASAPST; }
    inline void setASAPST(algorithm::PathTimes newASAPST) { m_ASAPST = std::move(newASAPST); }

    [[nodiscard]] inline const auto &getJobsCompletion() const noexcept { return m_jobsCompletion; }
    [[nodiscard]] inline auto &getJobsCompletion() noexcept { return m_jobsCompletion; }

    [[nodiscard]] inline bool getTerminal() const noexcept { return m_terminal; }
    inline void setTerminal(bool value) noexcept { m_terminal = value; }

    [[nodiscard]] inline const auto &getJobOrder() const noexcept { return m_jobOrder; }
    inline void setJobOrder(std::vector<FORPFSSPSD::JobId> newJobOrder) {
        m_jobOrder = std::move(newJobOrder);
    }

    /**
     * @brief Get the last vertex of each machine
     */
    [[nodiscard]] inline const auto &getLastOperation() const noexcept { return m_lastOperation; }
    inline void setLastOperation(MachineToVertex lastOperation) {
        m_lastOperation = std::move(lastOperation);
    }

    inline void setReadyOperations(FORPFSSPSD::JobOperations readyOps) {
        m_readyOps = std::move(readyOps);
    }

    inline void removeReadyOperation(FORPFSSPSD::JobId id) { m_readyOps.extract(id); }

    // void removeReadyOperation(const FORPFSSPSD::operation &op) {
    //     m_readyOps.erase(std::remove(m_readyOps.begin(),m_readyOps.end(),op),m_readyOps.end());}

    void setReadyOperations(const FORPFSSPSD::Instance &problemInstance,
                            bool graphIsRelaxed = false) {
        m_readyOps.clear();
        const auto &jobs = problemInstance.jobs();
        const auto &jobsOutput = problemInstance.getJobsOutput();

        // For all jobs find the possible operations that can be scheduled
        for (std::size_t i = 0; i < jobs.size(); ++i) {
            // jobsOutput contains all IDs of jobs. The order of jobsOutput only matters for the
            // fixed output order flow-shop
            const auto jobId = jobsOutput[i];
            const auto opIdx = m_jobsCompletion[i];
            const auto &jobOps = jobs.at(jobId); // no chosen permutation for any shop

            if (opIdx >= jobOps.size()) {
                continue;
            }
            // if fixed order flowshop with no overtaking, then previous job must have at least done
            // that operation index too
            if (problemInstance.shopType() == ShopType::FIXEDORDERSHOP && i > 0) {
                if (m_jobsCompletion[i - 1] <= opIdx) { // TODO: Mixed plexity breaks this
                    continue;
                }
            }

            // generalise no overtaking in flowshop
            // when we relax the graph, we allow overtaking because merge loses information on job
            // ordering
            //  and we do not want to exclude any solutions
            if (problemInstance.shopType() == ShopType::FLOWSHOP && opIdx > 0 && !graphIsRelaxed) {
                const auto &jobOrder = m_jobOrder;
                // this is a flowshop and an operation of this job has been scheduled at this state
                // and thus a relative order established
                auto pos = std::find(jobOrder.begin(), jobOrder.end(), jobId);
                if (pos != jobOrder.end() && pos - jobOrder.begin() > 0) {
                    if (m_jobsCompletion[jobOrder.at(pos - jobOrder.begin() - 1)]
                        <= m_jobsCompletion[jobOrder.at(pos - jobOrder.begin())]) {
                        continue;
                    }
                }
            }
            // Add the operation to ready queue
            // LOG(FMT_COMPILE("Adding operation {} of job {} to the queue"), opIdx, i);
            // in a permutation flowshop, there is no overlap so all operations of a job can be
            // ready at once, it is based on job order
            m_readyOps[jobId] = problemInstance.shopType() == ShopType::FLOWSHOP
                                        ? jobOps
                                        : FORPFSSPSD::OperationsVector{jobOps[opIdx]};
        }
        // LOG("End of ready queue at this time\n\n");
    }

private:
    std::uint64_t m_id;

    /// Selected edges per machine
    MachineEdges m_machineEdges;

    /// Current known earliest start times
    algorithm::PathTimes m_ASAPST;

    /// Current known latest start times
    algorithm::PathTimes m_ALAPST;

    /// Index of the next operation to do for each job. The index is not an OperationId but an index
    /// in the vector of operations of the job. The same goes for the index of the job which is the
    /// index of the job in the output order of the jobs.
    JobIdxToOpIdx m_jobsCompletion;

    // True if state is a terminal state i.e. all operations of all jobs have been scheduled
    bool m_terminal = false;

    /// Index of the job ordering, used in state expansion when no overtaking is allowed
    /// Job order inferred and filled from the relationship between initial operations of jobs in
    /// that state Immaterial for job shops unless no overtaking specified (case currently not
    /// considered), important for flowshops to obey no overtaking
    std::vector<FORPFSSPSD::JobId> m_jobOrder;

    /// Next ready operations from this state (feasible set)
    FORPFSSPSD::JobOperations m_readyOps;

    /// Operations already scheduled in this state
    DelayGraph::VerticesIDs m_scheduledOps;

    /// Last operation on each machine
    MachineToVertex m_lastOperation;

    /// @brief Operations already scheduled in this state
    /// @details In the full decision diagram, they are exactly equal to m_scheduledOps. It takes on
    /// meaning in the relaxed decision diagram when we merge. It is the union of scheduled ops of
    /// all states that were merged to create new state
    DelayGraph::VerticesIDs m_encounteredOps;

    /// Vertex depth to use for node selection
    std::uint64_t m_vertexDepth;
};
} // namespace DD

template <> struct std::hash<DD::JobIdxToOpIdx> {
    std::size_t operator()(const DD::JobIdxToOpIdx &k) const {
        std::size_t result = 0;
        for (const auto &i : k) {
            algorithm::hash_combine(result, i);
        }
        return result;
    }
};

#endif // DD_VERTEX_HPP