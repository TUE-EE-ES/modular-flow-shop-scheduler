#ifndef SOLVERS_DD_HPP
#define SOLVERS_DD_HPP

#include "DD/dd_solution.hpp"
#include "DD/vertex.hpp"
#include "FORPFSSPSD/FORPFSSPSD.h"
#include "delayGraph/delayGraph.h"
#include "delayGraph/edge.h"
#include "solver.h"
#include "solvers/solver_data.hpp"

#include <memory>

namespace algorithm {

/**
 * @brief The DD solver builds a Schedule Abstraction Graph of the current problem and
 *        finds the optimal solution by traversing the graph.
 */
class DDSolver {
public:
    class TerminationStrings {
    public:
        static constexpr auto kTimeOut = "time-out";
        static constexpr auto kNoSolution = "no-solution";
        static constexpr auto kOptimal = "optimal";
    };

    /**
     * @brief Stores the data used by the DD solver to resume the search later.
     * @tparam T Queue type used by the solver.
     */
    template <typename T> struct DDSolverData : public FMS::SolverData {
        T states;
        DD::DDSolution solution;
        DelayGraph::delayGraph dg;
        bool keepActiveVerticesSparse = true;
        std::uint64_t vertexId = 0;

        DDSolverData(T states,
                     DD::DDSolution solution,
                     DelayGraph::delayGraph dg,
                     bool keepActiveVerticesSparse,
                     std::uint64_t vertexId) :
            states(std::move(states)),
            solution(std::move(solution)),
            dg(std::move(dg)),
            keepActiveVerticesSparse(keepActiveVerticesSparse),
            vertexId(vertexId) {}
        DDSolverData(DDSolverData &&) = default;
        DDSolverData(const DDSolverData &) = default;
        
        ~DDSolverData() override = default;

        DDSolverData &operator=(DDSolverData &&) = default;
        DDSolverData &operator=(const DDSolverData &) = default;
    };

    template <typename T> using DDSolverDataPtr = std::unique_ptr<DDSolverData<T>>;

    using JobIdxToVertices =
            std::unordered_map<DD::JobIdxToOpIdx,
                               std::unordered_map<std::uint64_t, std::shared_ptr<DD::Vertex>>>;
    using IdToVertex = std::unordered_map<std::uint64_t, std::shared_ptr<DD::Vertex>>;

    float rankFactor;
    std::uint32_t totalOps;

    /**
     * @brief Solve the passed problem instance and return the sequences of operations per
     * machine.
     * @param problemInstance Instance of a n-re-entrant flow-shop problem with setup times,
     *                        due dates, and/or maintenance times.
     * @param args Command line arguments.
     */

    [[nodiscard]] static std::tuple<Solutions, nlohmann::json>
    solve(FORPFSSPSD::Instance &problemInstance, const commandLineArgs &args);

    template <typename T>
    [[nodiscard]] static ResumableSolverOutput solveWrap(FORPFSSPSD::Instance &problemInstance,
                                                         const commandLineArgs &args,
                                                         DDSolverDataPtr<T> oldDataPtr);

    [[nodiscard]] static ResumableSolverOutput
    solveResumable(FORPFSSPSD::Instance &problemInstance,
                   FORPFSSPSD::ProblemUpdate problemUpdate,
                   const commandLineArgs &args,
                   FMS::SolverDataPtr solverData);

    [[nodiscard]] static Solutions
    extractSolutions(const std::vector<DD::Vertex> &statesTerminated);

    static void removeActiveVertex(DDSolver::JobIdxToVertices &activeVertices, const DD::Vertex &v);

    [[nodiscard]] static DD::SharedVertex
    createNewVertex(std::uint64_t &vertexId,
                    const DD::Vertex &oldVertex,
                    const FORPFSSPSD::Instance &problemInstance,
                    const DelayGraph::VerticesIDs &vOps,
                    const std::vector<FORPFSSPSD::operation> &ops,
                    PathTimes ASAPST,
                    PathTimes ALAPST,
                    DelayGraph::Edges edges,
                    bool graphIsRelaxed = false);

    static std::vector<DD::SharedVertex>
    expandVertex(auto &data, const DD::Vertex &state, const FORPFSSPSD::Instance &problemInstance);

    /**
     * @brief Finds if the state can be merged with another state in the graph and returns the
     * merged state or nullptr if the state is dominated by others.
     *
     * @param activeVertices The set of active (known) vertices in the graph.
     * @param newState The state to be merged.
     * @return true The vertex is not dominated
     * @return false The vertex id dominated and it should not be added
     */
    [[nodiscard]] static bool findVertexDominance(JobIdxToVertices &activeVertices,
                                                  const DD::SharedVertex &newVertex,
                                                  const FORPFSSPSD::Instance &problemInstance);

    /**
     * @brief Checks if @p newVertex is dominated by @p oldVertex .
     *
     * @param newVertex DD vertex to be checked.
     * @param oldVertex DD vertex to be checked against.
     * @return true @p newVertex is dominated by @p oldVertex .
     * @return false @p newVertex is not dominated by @p oldVertex .
     */
    [[nodiscard]] static bool isDominated(const DD::Vertex &newVertex,
                                          const DD::Vertex &oldVertex,
                                          const FORPFSSPSD::Instance &problemInstance);

    [[nodiscard]] static bool isTerminal(const DD::Vertex &vertex,
                                         const FORPFSSPSD::Instance &instance);

    // NOTE: This is not being used for the current paper but may be used in future work
    template <typename T, class F = void *>
    static void mergeLoop(T &states,
                          std::uint64_t &vertexId,
                          const FORPFSSPSD::Instance &problemInstance,
                          const DelayGraph::delayGraph &dg);

    static DD::SharedVertex mergeOperator(const DD::Vertex &a,
                                          const DD::Vertex &b,
                                          std::uint64_t &vertexId,
                                          const FORPFSSPSD::Instance &problemInstance,
                                          const DelayGraph::delayGraph &dg);

    static std::uint32_t chooseVertexToMerge(std::uint32_t size);

    static void updateVertexALAPST(const PathTimes &ASAPST,
                                   PathTimes &ALAPST,
                                   DelayGraph::delayGraph &dg,
                                   const DelayGraph::VerticesIDs &scheduledOps,
                                   const DelayGraph::Edges &newestEdges,
                                   const std::vector<FORPFSSPSD::operation> &newestOps);

    static void updateVertexALAPST(const PathTimes &ASAPST,
                                   PathTimes &ALAPST,
                                   DelayGraph::delayGraph &dg,
                                   const DelayGraph::VerticesIDs &scheduledOps);

    static DelayGraph::Edges inferEdges(const DD::Vertex &s,
                                        const FORPFSSPSD::Instance &problemInstance,
                                        const DelayGraph::delayGraph &dg);

    [[nodiscard]] static PartialSolution getSeedSolution(FORPFSSPSD::Instance &problemInstance,
                                                         const commandLineArgs &args);

    template <typename T>
    static void initialiseStates(const FORPFSSPSD::Instance &problemInstance,
                                 T &states,
                                 const commandLineArgs &args,
                                 DelayGraph::delayGraph &dg,
                                 PartialSolution seedSolution,
                                 std::uint64_t &vertexId,
                                 DD::SharedVertex &oldVertex,
                                 DD::DDSolution &solution);

    template <typename T>
    static void createSeedSolutionStates(
            const FORPFSSPSD::Instance &problemInstance,
            T &states,
            const commandLineArgs &args,
            DelayGraph::delayGraph &dg,
            const std::unordered_map<FORPFSSPSD::MachineId, DelayGraph::Edges> &seedSolutionEdges,
            std::uint64_t &vertexId,
            DD::SharedVertex oldVertex,
            const DD::DDSolution &solution,
            const std::vector<FORPFSSPSD::MachineId> &machineOrder);
    template <typename T>
    static void createSeedSolutionStates(const FORPFSSPSD::Instance &problemInstance,
                                         T &states,
                                         const commandLineArgs &args,
                                         DelayGraph::delayGraph &dg,
                                         std::uint64_t &vertexId,
                                         DD::SharedVertex oldVertex,
                                         DD::DDSolution &solution,
                                         std::vector<FORPFSSPSD::JobId> &jobOrder);

    [[nodiscard]] static std::tuple<DelayGraph::Edges, DelayGraph::VerticesIDs>
    createSchedulingOptionEdges(const FORPFSSPSD::Instance &problemInstance,
                                const DelayGraph::delayGraph &dg,
                                const DD::Vertex &oldVertex,
                                const std::vector<FORPFSSPSD::operation> &ops);

    static DD::SharedVertex popQueue(std::deque<DD::SharedVertex> &states);
    static DD::SharedVertex popQueue(std::vector<DD::SharedVertex> & /*states*/) {
        throw std::runtime_error("FmsScheduler::should only call this function for non static or "
                                 "non adaptive search");
    }

    template <class F>
    static DD::SharedVertex popQueue(std::vector<DD::SharedVertex> &states, F comparator);
    template <class F>
    static DD::SharedVertex popQueue(std::deque<DD::SharedVertex> & /*states*/, F /*comparator*/) {
        throw std::runtime_error(
                "FmsScheduler::should only call this function for static or adaptive search");
    }

    template <class V>
    static void pushQueue(std::vector<DD::SharedVertex> &states, DD::SharedVertex &s, V comparator);
    template <class V>
    static void pushQueue(std::deque<DD::SharedVertex> & /*states*/,
                          DD::SharedVertex & /*s*/,
                          V /*comparator*/) {
        throw std::runtime_error("FmsScheduler::should only call this function for static or "
                                 "adaptive or best search");
    }

    static void pushQueue(std::deque<DD::SharedVertex> &states,
                          DD::SharedVertex &s,
                          DDExplorationType comparator);
    static void pushQueue(std::vector<DD::SharedVertex> & /*states*/,
                          DD::SharedVertex & /*s*/,
                          DDExplorationType /*comparator*/) {
        throw std::runtime_error(
                "FmsScheduler::should only call this function for breadth or depth search");
    }

    template <class V> static void orderQueue(std::vector<DD::SharedVertex> &states, V comparator);
    template <class V>
    static void orderQueue(std::deque<DD::SharedVertex> & /*states*/, V /*comparator*/) {
        throw std::runtime_error(
                "FmsScheduler::should only call this function for static or adaptive search");
    }

    template <typename T>
    static void inline push(DDSolverData<T> &data,
                            DDExplorationType explorationType,
                            DD::SharedVertex &newVertex,
                            bool reOrder) {
        push(data.states, explorationType, newVertex, data.solution, reOrder);
    }
    static void push(auto &states,
                     DDExplorationType explorationType,
                     DD::SharedVertex &newVertex,
                     const DD::DDSolution &solution,
                     bool reOrder);

    template <typename T>
    static DD::SharedVertex pop(DDSolverData<T> &data, DDExplorationType explorationType);

    template <typename T>
    void updateProblem(FORPFSSPSD::Instance &problemInstance,
                       FORPFSSPSD::ProblemUpdate problemUpdate,
                       DDSolverDataPtr<T> &dataPtr,
                       const commandLineArgs &args);
};

} // namespace algorithm

#endif // SOLVERS_DD_HPP