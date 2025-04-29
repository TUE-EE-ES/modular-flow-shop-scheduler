#ifndef FMS_SOLVERS_DD_HPP
#define FMS_SOLVERS_DD_HPP

#include "solver.hpp"
#include "solver_data.hpp"

#include "fms/cg/constraint_graph.hpp"
#include "fms/cg/edge.hpp"
#include "fms/dd/dd_solution.hpp"
#include "fms/dd/vertex.hpp"
#include "fms/problem/flow_shop.hpp"

#include <memory>
#include <nlohmann/json.hpp>
#include <vector>

namespace fms::solvers {

/**
 * @brief The DD solver builds a Schedule Abstraction Graph of the current problem and
 *        finds the optimal solution by traversing the graph.
 */
namespace dd {
using namespace fms::dd;
using JobIdxToVertices =
        std::unordered_map<JobIdxToOpIdx,
                           std::unordered_map<std::uint64_t, std::shared_ptr<Vertex>>>;
using IdToVertex = std::unordered_map<std::uint64_t, std::shared_ptr<Vertex>>;
using StatesT = std::deque<SharedVertex>;

struct TerminationStrings {
    static constexpr auto kTimeOut = "time-out";
    static constexpr auto kNoSolution = "no-solution";
    static constexpr auto kOptimal = "optimal";
};

constexpr auto kStoreHistory = "store-history";

/**
 * @brief Stores the data used by the DD solver to resume the search later.
 * @tparam T Queue type used by the solver.
 */
struct DDSolverData : public SolverData {
    /// Queue of states to be explored by the algorithm
    StatesT states;

    /// All states that have been explored
    std::deque<SharedVertex> allStates;

    /// Next vertex id to be used
    std::uint64_t nextVertexId;

    DDSolution solution;
    cg::ConstraintGraph dg;

    cli::DDExplorationType explorationType;
    bool keepActiveVerticesSparse;
    bool storeAllStates;

    // To check for possible merging we keep track the current "active" vertices in a searchable
    // data structure. The active vertices are the ones still inside the queue. The weak_ptr
    // tells us if the vertex is still alive.
    dd::JobIdxToVertices activeVertices;

    DDSolverData(cli::DDExplorationType explorationType,
                 DDSolution solution,
                 cg::ConstraintGraph dg,
                 bool keepActiveVerticesSparse,
                 bool storeAllStates = false,
                 StatesT states = {},
                 std::deque<SharedVertex> allStates = {},
                 std::uint64_t nextVertexId = 0) :
        states(std::move(states)),
        allStates(std::move(allStates)),
        nextVertexId(nextVertexId),
        solution(std::move(solution)),
        dg(std::move(dg)),
        explorationType(explorationType),
        keepActiveVerticesSparse(keepActiveVerticesSparse),
        storeAllStates(storeAllStates) {}
    DDSolverData(DDSolverData &&) noexcept = default;
    DDSolverData(const DDSolverData &) = default;

    ~DDSolverData() override = default;

    DDSolverData &operator=(DDSolverData &&) = default;
    DDSolverData &operator=(const DDSolverData &) = default;

    void storeState(const SharedVertex &newVertex);
};

using DDSolverDataPtr = std::unique_ptr<DDSolverData>;

/**
 * @brief Solve the passed problem instance and return the sequences of operations per
 * machine.
 * @param problemInstance Instance of a n-re-entrant flow-shop problem with setup times,
 *                        due dates, and/or maintenance times.
 * @param args Command line arguments.
 */

[[nodiscard]] std::tuple<Solutions, nlohmann::json> solve(problem::Instance &problemInstance,
                                                          const cli::CLIArgs &args);

DDSolverDataPtr
initialize(const cli::CLIArgs &args, problem::Instance &instance, DDSolverDataPtr dataOld);

[[nodiscard]] ResumableSolverOutput
solveWrap(problem::Instance &problemInstance, const cli::CLIArgs &args, DDSolverDataPtr oldDataPtr);

[[nodiscard]] ResumableSolverOutput solveResumable(problem::Instance &problemInstance,
                                                   problem::ProblemUpdate problemUpdate,
                                                   const cli::CLIArgs &args,
                                                   SolverDataPtr solverData);

[[nodiscard]] ResumableSolverOutput solveTerminate(DDSolverDataPtr data);

[[nodiscard]] Solutions extractSolutions(const std::vector<Vertex> &statesTerminated);

void removeActiveVertex(dd::JobIdxToVertices &activeVertices, const Vertex &v);

[[nodiscard]] SharedVertex createNewVertex(std::uint64_t &vertexId,
                                           const Vertex &oldVertex,
                                           const problem::Instance &problemInstance,
                                           const cg::VerticesIds &vOps,
                                           const std::vector<problem::Operation> &ops,
                                           algorithms::paths::PathTimes ASAPST,
                                           algorithms::paths::PathTimes ALAPST,
                                           bool graphIsRelaxed = false);

std::vector<SharedVertex>
expandVertex(auto &data, const Vertex &state, const problem::Instance &problemInstance);

/**
 * @brief Finds if the state can be merged with another state in the graph and returns the
 * merged state or nullptr if the state is dominated by others.
 *
 * @param activeVertices The set of active (known) vertices in the graph.
 * @param newVertex The whose dominance we want to check.
 * @param problemInstance The problem instance.
 * @return true The vertex @p newVertex is not dominated.
 * @return false The vertex @p newVertex is dominated and it should not be added.
 */
[[nodiscard]] bool findVertexDominance(JobIdxToVertices &activeVertices,
                                       const SharedVertex &newVertex,
                                       const problem::Instance &problemInstance);

/**
 * @brief Checks if @p newVertex is dominated by @p oldVertex .
 *
 * @param newVertex DD vertex to be checked.
 * @param oldVertex DD vertex to be checked against.
 * @param problemInstance The problem instance. It must be the same as the one used to create the
 * vertices.
 * @return true @p newVertex is dominated by @p oldVertex .
 * @return false @p newVertex is not dominated by @p oldVertex .
 */
[[nodiscard]] bool isDominated(const Vertex &newVertex,
                               const Vertex &oldVertex,
                               const problem::Instance &problemInstance);

[[nodiscard]] bool isTerminal(const Vertex &vertex, const problem::Instance &instance);

// NOTE: This is not being used for the current paper but may be used in future work
template <typename T, class F = void *>
void mergeLoop(T &states,
               std::uint64_t &vertexId,
               const problem::Instance &problemInstance,
               const cg::ConstraintGraph &dg);

SharedVertex mergeOperator(const Vertex &a,
                           const Vertex &b,
                           std::uint64_t &vertexId,
                           const problem::Instance &problemInstance,
                           const cg::ConstraintGraph &dg);

std::uint32_t chooseVertexToMerge(std::uint32_t size);

void updateVertexALAPST(const algorithms::paths::PathTimes &ASAPST,
                        algorithms::paths::PathTimes &ALAPST,
                        cg::ConstraintGraph &dg,
                        const cg::VerticesIds &scheduledOps,
                        const cg::Edges &newestEdges,
                        const std::vector<problem::Operation> &newestOps);

void updateVertexALAPST(const algorithms::paths::PathTimes &ASAPST,
                        algorithms::paths::PathTimes &ALAPST,
                        cg::ConstraintGraph &dg,
                        const cg::VerticesIds &scheduledOps);

cg::Edges inferEdges(const Vertex &s,
                         const problem::Instance &problemInstance,
                         const cg::ConstraintGraph &dg);

[[nodiscard]] PartialSolution getSeedSolution(problem::Instance &problemInstance,
                                              const cli::CLIArgs &args);

void initialiseStates(const problem::Instance &problemInstance,
                      DDSolverData &data,
                      const cli::CLIArgs &args,
                      PartialSolution seedSolution,
                      SharedVertex &rootVertex);

[[nodiscard]] std::tuple<cg::Edges, cg::VerticesIds>
createSchedulingOptionEdges(const problem::Instance &problemInstance,
                            const cg::ConstraintGraph &dg,
                            const Vertex &oldVertex,
                            const std::vector<problem::Operation> &ops);

bool shouldStop(const dd::DDSolverData &data, const cli::CLIArgs &args, std::size_t iterations);

void singleIteration(DDSolverData &dataPtr, const problem::Instance &problemInstance);

template <class V> void orderQueue(StatesT &states, V comparator) {
    std::make_heap(states.begin(), states.end(), comparator);
}

void push(auto &states,
          cli::DDExplorationType explorationType,
          SharedVertex &newVertex,
          const DDSolution &solution,
          bool reOrder);

void inline push(DDSolverData &data, SharedVertex &newVertex, bool reOrder) {
    push(data.states, data.explorationType, newVertex, data.solution, reOrder);
}

SharedVertex pop(DDSolverData &data);
}; // namespace dd

} // namespace fms::solvers

#endif // FMS_SOLVERS_DD_HPP
