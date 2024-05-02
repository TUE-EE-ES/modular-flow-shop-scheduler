#include "solvers/dd.hpp"

#include "DD/comparator.hpp"
#include "DD/vertex.hpp"
#include "FORPFSSPSD/FORPFSSPSD.h"
#include "FORPFSSPSD/indices.hpp"
#include "delayGraph/builder.hpp"
#include "delayGraph/delayGraph.h"
#include "delayGraph/edge.h"
#include "delayGraph/export_utilities.h"
#include "solvers/utils.hpp"

#include "pch/utils.hpp"
#include "solvers/sequence.hpp"

#include <algorithm>
#include <fmt/chrono.h>
#include <fmt/compile.h>

using namespace algorithm;
using a_clock = std::chrono::steady_clock;
using SharedVertex = std::shared_ptr<DD::Vertex>; // remove after templating solved

static constexpr float kDefaultRankFactor = 0.8;

namespace {

template <typename T>
auto initialize(const commandLineArgs &args, FORPFSSPSD::Instance &problemInstance) {
    DD::DDSolution solution(FMS::getCpuTime(), kDefaultRankFactor, problemInstance.getTotalOps());

    // Generate the base graph
    auto dg = DelayGraph::Builder::jobShop(problemInstance);
    problemInstance.updateDelayGraph(dg);

    if (args.verbose >= LOGGER_LEVEL::DEBUG) {
        auto name = fmt::format("input_delayGraph_{}.dot", problemInstance.getProblemName());
        DelayGraph::export_utilities::saveAsDot(dg, name);
    }

    auto [result, ASAPST] = SolversUtils::checkSolutionAndOutputIfFails(problemInstance);
    auto [_, ALAPST] = LongestPath::computeALAPST(dg);

    LOG("Number of vertices in the delay graph is {} ", dg.get_number_of_vertices());
    LOG("ASAPST is of size {} with the last operation being ({}) started at {}.",
        ASAPST.size(),
        dg.get_vertex(ASAPST.size() - 1).operation,
        ASAPST.back());

    const auto &jobs = problemInstance.jobs();
    const auto &machines = problemInstance.getMachines();

    T states;

    std::uint64_t vertexId = 0;
    // Initialize the root state
    auto root = std::make_shared<DD::Vertex>(vertexId++,
                                             DD::MachineEdges{},
                                             ASAPST,
                                             ALAPST,
                                             DD::JobIdxToOpIdx(jobs.size(), 0),
                                             std::vector<FORPFSSPSD::JobId>{},
                                             DD::MachineToVertex{},
                                             DelayGraph::VerticesIDs{});
    root->setReadyOperations(problemInstance);

    bool keepActiveVerticesSparse = true;

    // get seed solution if available
    if (!args.sequenceFile.empty()) {
        auto seedSolution = DDSolver::getSeedSolution(problemInstance, args);

        // create end vertex of solution
        auto seedVertex = std::make_shared<DD::Vertex>(vertexId++,
                                                       seedSolution.getChosenEdgesPerMachine(),
                                                       seedSolution.getASAPST(),
                                                       seedSolution.getASAPST(),
                                                       DD::JobIdxToOpIdx(jobs.size(), 0),
                                                       std::vector<FORPFSSPSD::JobId>{},
                                                       DD::MachineToVertex{},
                                                       DelayGraph::VerticesIDs{},
                                                       seedSolution.getASAPST().back());

        // add seed solution to solutions
        solution.addNewSolution(*seedVertex);
        LOG("best upper bound from heuristic is {}", solution.bestUpperBound());

        // create vertices for every state of solution to continue to build from
        // add seed vertices to active vertices
        // add all children of seed vertices to states queue
        DDSolver::initialiseStates(
                problemInstance, states, args, dg, seedSolution, vertexId, root, solution);
        keepActiveVerticesSparse = false;
    }

    // add root to queue
    DDSolver::push(states, args.explorationType, root, solution, true);

    return std::make_tuple(std::move(states),
                           std::move(solution),
                           std::move(dg),
                           keepActiveVerticesSparse,
                           vertexId);
}

inline bool shouldStop(const auto &states,
                       const DD::DDSolution &solution,
                       const commandLineArgs &args,
                       const std::size_t iterations) {
    return states.empty() || (FMS::getCpuTime() - solution.start()) >= args.timeOut
           || iterations >= args.maxIterations;
}
} // namespace

std::tuple<Solutions, nlohmann::json> DDSolver::solve(FORPFSSPSD::Instance &problemInstance,
                                                      const commandLineArgs &args) {
    switch (args.explorationType) {
    case DDExplorationType::BREADTH:
    case DDExplorationType::DEPTH:
        return DDSolver::solveWrap<std::deque<SharedVertex>>(problemInstance, args);

    case DDExplorationType::BEST:
    case DDExplorationType::STATIC:
    case DDExplorationType::ADAPTIVE:
        return DDSolver::solveWrap<std::vector<SharedVertex>>(problemInstance, args);

    default:
        throw std::runtime_error("FmsScheduler::unknown graph exploration heuristic supplied");
    }
}

template <typename T>
std::tuple<Solutions, nlohmann::json> DDSolver::solveWrap(FORPFSSPSD::Instance &problemInstance,
                                                          const commandLineArgs &args) {

    LOG("Computation of the schedule using DD started");

    auto [states, solution, dg, keepActiveVerticesSparse, vertexId] =
            ::initialize<T>(args, problemInstance);

    // To check for possible merging we keep track the current "active" vertices in a searchable
    // data structure. The active vertices are the ones still inside the queue. The weak_ptr
    // tells us if the vertex is still alive.
    DDSolver::JobIdxToVertices activeVertices;

    // Count iterations
    std::uint32_t iterations = 0;

    while (!shouldStop(states, solution, args, iterations)) {

        SharedVertex s = pop(states, args.explorationType, solution);

        const auto &sMachineEdges = s->getMachineEdges();
        const auto &sLastOperation = s->getLastOperation();
        const auto currentDepth = s->vertexDepth();

        // The vertex becomes inactive after expansion so we remove it
        if (keepActiveVerticesSparse) {
            removeActiveVertex(activeVertices, *s);
        }

        if (isTerminal(*s, problemInstance)) {
            // if terminal we should update upper bound and add state to another queue otherwise we
            // lose it
            solution.addNewSolution(*s);
            if (solution.isOptimal()) {
                LOG("Solution is optimal");
                break;
            }
            continue;
        }
        const auto addedEdges = dg.add_edges(s->getAllEdges());

        LOG("Expanding state");
        auto expandedStates = expandVertex(vertexId, *s, dg, problemInstance);

        // For each ready operation(s), attempt to schedule and extend to a new state
        // If dominated, discard
        // If dominates, replace and propagate release to child nodes
        for (auto newState : expandedStates) {

            // First check if it is dominated upper bound wise and discard if it is
            // Then check if it dominates another state or it is not dominated we create a new state
            // and add it. Otherwise we discard it. These checks have to happen in this order
            // otherwise findandmerge could add a vertex to active vertices and it ends up discarded
            // causing a pointer error.

            if (newState->lowerBound() > solution.bestUpperBound()) {
                continue;
            }
            if (findVertexDominance(activeVertices, newState, problemInstance)) {
                continue;
            }

            push(states, args.explorationType, newState, solution, false);
        }

        // update best lower bound
        auto bestLowerBoundElement =
                std::min_element(states.begin(), states.end(), DD::CompareVerticesLowerBoundMin());
        auto minLowerBound = bestLowerBoundElement == states.end()
                                     ? solution.bestUpperBound()
                                     : std::min((*bestLowerBoundElement)->lowerBound(),
                                                solution.bestUpperBound());
        LOG_D("Lower is {} and upper is {}", minLowerBound, solution.bestUpperBound());
        solution.setBestLowerBound(minLowerBound);

        // Remove the edges that were added
        dg.remove_edges(addedEdges);

        LOG_D("Queue is {} elements long", states.size());

        if (solution.isOptimal()) {
            LOG("Solution is optimal");
            break;
        }

        if (states.empty()) {
            LOG("States is empty");
        }
        iterations++;
    }

    auto data = solution.getSolveData();

    // Find termination reason
    if (solution.isOptimal()) {
        data["terminationReason"] = TerminationStrings::kOptimal;
    } else if (states.empty()) {
        data["terminationReason"] = TerminationStrings::kNoSolution;
    } else {
        data["terminationReason"] = TerminationStrings::kTimeOut;
        // The other two reasons already print a message inside the loop
        LOG("DD: Time out");
    }

    return {DDSolver::extractSolutions(solution.getStatesTerminated()), std::move(data)};
}

PartialSolution algorithm::DDSolver::getSeedSolution(FORPFSSPSD::Instance &problemInstance,
                                                     const commandLineArgs &args) {
    return std::get<0>(algorithm::Sequence::solve(problemInstance, args)).front();
}

std::tuple<DelayGraph::Edges, DelayGraph::VerticesIDs>
DDSolver::createSchedulingOptionEdges(const FORPFSSPSD::Instance &problemInstance,
                                      const DelayGraph::delayGraph &dg,
                                      const DD::Vertex &oldVertex,
                                      const std::vector<FORPFSSPSD::operation> &ops) {
    DelayGraph::Edges newEdges{};
    DelayGraph::VerticesIDs readyVIDs{};

    newEdges.reserve(ops.size());
    readyVIDs.reserve(ops.size());

    const auto &stateLastOperation = oldVertex.getLastOperation();
    for (auto op : ops) {
        const auto mId = problemInstance.getMachine(op);
        const auto &v = dg.get_vertex(op);

        const auto &lastOp = stateLastOperation.find(mId);

        const auto &lastVertex = lastOp == stateLastOperation.end() ? dg.get_source(mId)
                                                                    : dg.get_vertex(lastOp->second);

        newEdges.emplace_back(lastVertex.id, v.id, problemInstance.query(lastVertex, v));
        readyVIDs.push_back(v.id);
    }
    return {std::move(newEdges), std::move(readyVIDs)};
}

template <typename T>
void DDSolver::initialiseStates(const FORPFSSPSD::Instance &problemInstance,
                                T &states,
                                const commandLineArgs &args,
                                DelayGraph::delayGraph &dg,
                                PartialSolution seedSolution,
                                std::uint64_t &vertexId,
                                DDSolver::SharedVertex &oldVertex,
                                DD::DDSolution &solution) {
    const auto &seedSolutionEdges = seedSolution.getChosenEdgesPerMachine();

    if (problemInstance.shopType() == ShopType::FLOWSHOP) {
        std::vector<FORPFSSPSD::JobId> jobOrder;
        for (const auto &e : (seedSolutionEdges.begin())->second) {
            jobOrder.push_back(dg.get_vertex(e.dst).operation.jobId);
        }
        createSeedSolutionStates(
                problemInstance, states, args, dg, vertexId, oldVertex, solution, jobOrder);
    } else {
        // sort seed solution edges by machine index
        std::vector<FORPFSSPSD::MachineId> machineOrder;
        machineOrder.reserve(seedSolutionEdges.size());
        for (const auto &it : seedSolutionEdges) {
            machineOrder.push_back(it.first);
        }
        std::sort(machineOrder.begin(), machineOrder.end());

        createSeedSolutionStates(problemInstance,
                                 states,
                                 args,
                                 dg,
                                 seedSolutionEdges,
                                 vertexId,
                                 oldVertex,
                                 solution,
                                 machineOrder);
    }
}

template <typename T>
void DDSolver::createSeedSolutionStates(
        const FORPFSSPSD::Instance &problemInstance,
        T &states,
        const commandLineArgs &args,
        DelayGraph::delayGraph &dg,
        const std::unordered_map<FORPFSSPSD::MachineId, DelayGraph::Edges> &seedSolutionEdges,
        std::uint64_t &vertexId,
        DDSolver::SharedVertex oldVertex,
        const DD::DDSolution &solution,
        const std::vector<FORPFSSPSD::MachineId> &machineOrder) {
    DelayGraph::Edges stateEdges = oldVertex->getAllEdges();
    dg.add_edges(stateEdges);

    for (const auto &mId : machineOrder) {
        const auto &edges = seedSolutionEdges.at(mId);
        for (auto edge : edges) {

            auto stateASAPST = oldVertex->getASAPST();
            auto stateALAPST = oldVertex->getALAPST();

            auto vOp = edge.dst;
            auto op = dg.get_vertex(vOp).operation;

            auto inferredEdges = inferEdges(*oldVertex, problemInstance, dg);
            // DelayGraph::Edges inferredEdges = {};
            inferredEdges.push_back(edge);

            // // Check if updating the path with a new edge is feasible while also inferring lower
            // bound
            if (algorithm::LongestPath::addEdgesIncrementalASAPST(dg, inferredEdges, stateASAPST)) {
                // No, adding one edge creates a cycle - simultaneously updates ASAPST
                // Throw error because seed solution should always be feasible
                throw FmsSchedulerException("The seed solution is infeasible");
            }

            DDSolver::updateVertexALAPST(
                    stateASAPST, stateALAPST, dg, oldVertex->scheduledOps(), {edge}, {op});

            dg.add_edge(edge);
            stateEdges.push_back(edge);

            auto newVertex = DDSolver::createNewVertex(vertexId,
                                                       *oldVertex,
                                                       problemInstance,
                                                       {vOp},
                                                       {op},
                                                       std::move(stateASAPST),
                                                       std::move(stateALAPST),
                                                       {edge});
            newVertex->setReadyOperations(problemInstance, false);

            // remove ready op from old vertex
            oldVertex->removeReadyOperation(op.jobId);

            LOG(FMT_COMPILE("Adding operation {} of job {} to seed solution path"),
                op.operationId,
                op.jobId);

            // add seed state to queue
            push(states, args.explorationType, newVertex, solution, true);

            oldVertex = newVertex;
        }
    }
    dg.remove_edges(stateEdges);
}

template <typename T>
void DDSolver::createSeedSolutionStates(const FORPFSSPSD::Instance &problemInstance,
                                        T &states,
                                        const commandLineArgs &args,
                                        DelayGraph::delayGraph &dg,
                                        std::uint64_t &vertexId,
                                        DDSolver::SharedVertex oldVertex,
                                        DD::DDSolution &solution,
                                        std::vector<FORPFSSPSD::JobId> &jobOrder) {
    DelayGraph::Edges stateEdges;
    for (auto &jId : jobOrder) {
        stateEdges = oldVertex->getAllEdges();
        dg.add_edges(stateEdges);
        auto stateASAPST = oldVertex->getASAPST();
        auto stateALAPST = oldVertex->getALAPST();
        auto stateLastOperation = oldVertex->getLastOperation();

        const auto &ops = problemInstance.jobs(jId);
        const auto &[newEdges, readyVIds] =
                createSchedulingOptionEdges(problemInstance, dg, *oldVertex, ops);

        // Update the path with a new edge is feasible
        if (LongestPath::addEdgesIncrementalASAPST(dg, newEdges, stateASAPST)) {
            // No, adding one edge creates a cycle - simultaneously updates ASAPST
            // Throw error because seed solution should always be feasible
            throw FmsSchedulerException("The seed solution is infeasible");
        }
        updateVertexALAPST(stateASAPST, stateALAPST, dg, oldVertex->scheduledOps(), newEdges, ops);

        auto newVertex = createNewVertex(vertexId,
                                         *oldVertex,
                                         problemInstance,
                                         readyVIds,
                                         ops,
                                         stateASAPST,
                                         stateALAPST,
                                         newEdges,
                                         false);
        // remove ready job from old vertex
        oldVertex->removeReadyOperation(jId);
        LOG(FMT_COMPILE("Adding all operations of job {} to seed solution path"), jId);

        // add seed state to queue
        push(states, args.explorationType, newVertex, solution, true);

        dg.remove_edges(stateEdges);
        oldVertex = newVertex;
    }
    LOG("Created seed solution states");
}

void algorithm::DDSolver::removeActiveVertex(DDSolver::JobIdxToVertices &activeVertices,
                                             const DD::Vertex &v) {
    auto it = activeVertices.find(v.getJobsCompletion());
    if (it == activeVertices.end()) {
        return;
    }

    it->second.erase(v.id());
    if (it->second.empty()) {
        activeVertices.erase(it);
    }
}

Solutions DDSolver::extractSolutions(const std::vector<DD::Vertex> &statesTerminated) {
    Solutions solutions;
    for (const auto &state : statesTerminated) {
        LOG("New Solution");
        for (const auto &[m, mEdges] : state.getMachineEdges()) {
            LOG(fmt::format(FMT_COMPILE("Machine {} has the following sequence"), m));
            for (auto e : mEdges) {
                LOG(fmt::to_string(e));
            }
        }
        PartialSolution solution(state.getMachineEdges(), state.getASAPST());
        solutions.push_back(solution);
    }
    LOG(FMT_COMPILE("Found {} solutions"), solutions.size());
    return solutions;
}

DelayGraph::Edges algorithm::DDSolver::inferEdges(const DD::Vertex &s,
                                                  const FORPFSSPSD::Instance &problem,
                                                  const DelayGraph::delayGraph &dg) {
    DelayGraph::Edges inferredEdges;

    const auto &machineEdges = s.getMachineEdges();
    const auto &lastOperation = s.getLastOperation();
    const auto &scheduledOps = s.scheduledOps();
    const std::unordered_set<DelayGraph::VertexID> scheduledOpsFast(scheduledOps.begin(),
                                                                    scheduledOps.end());

    // trying total processing time for an even tighter bound
    // TO DO: keep this value in the state and subtract everytime we make a new sched decision
    std::unordered_map<FORPFSSPSD::MachineId, delay> machineTotalTimeLeft;
    for (const auto &[opTo, mId] : problem.machineMapping()) {
        const auto vIdTo = dg.get_vertex_id(opTo);

        if (scheduledOpsFast.find(vIdTo) != scheduledOpsFast.end()) {
            continue;
        }

        // mapped to the same machine but not in scheduled ops
        const auto it2 = lastOperation.find(mId);
        delay time = problem.getProcessingTime(vIdTo);

        // weight is just the processing time for lower bound inference
        // calculate total time left on machine
        if (machineTotalTimeLeft.find(mId) == machineTotalTimeLeft.end()
            && it2 != lastOperation.end()) {
            time += problem.getProcessingTime(it2->second);
        }
        // STD guarantees default initialization to 0 for integral types on maps
        // (https://en.cppreference.com/w/cpp/language/zero_initialization)
        machineTotalTimeLeft[mId] += time;

        // add an edge for each operation in the future on same machine
        if (it2 == lastOperation.end()) {
            inferredEdges.emplace_back(dg.get_source(mId).id, vIdTo, 0);
        } else {
            inferredEdges.emplace_back(it2->second, vIdTo, problem.getProcessingTime(it2->second));
        }
    }

    // NOTE: Do we need this?
    auto terminalV = dg.get_terminus();
    for (auto [mId, time] : machineTotalTimeLeft) {
        const auto it2 = lastOperation.find(mId);
        // Insert it from the source if not found
        DelayGraph::VertexID vIdFrom =
                it2 == lastOperation.end() ? dg.get_source(mId).id : it2->second;
        inferredEdges.emplace_back(vIdFrom, terminalV.id, time);
    }
    return inferredEdges;
}

algorithm::DDSolver::SharedVertex
algorithm::DDSolver::createNewVertex(std::uint64_t &vertexId,
                                     const DD::Vertex &oldVertex,
                                     const FORPFSSPSD::Instance &problemInstance,
                                     const DelayGraph::VerticesIDs &vOps,
                                     const std::vector<FORPFSSPSD::operation> &ops,
                                     PathTimes ASAPST,
                                     PathTimes ALAPST,
                                     DelayGraph::Edges edges,
                                     const bool graphIsRelaxed) {

    auto newJobOrder = oldVertex.getJobOrder();
    auto newJobsCompletion = oldVertex.getJobsCompletion();
    auto newMachineEdges = oldVertex.getMachineEdges();
    auto newScheduledOps = oldVertex.scheduledOps();
    auto newLastOperation = oldVertex.getLastOperation();

    for (std::size_t i = 0; i < ops.size(); i++) {
        const auto &op = ops[i];
        const auto &mId = problemInstance.getMachine(op);
        newJobsCompletion[op.jobId] += 1;
        newMachineEdges[mId].push_back(edges[i]);
        newScheduledOps.push_back(vOps[i]);
        newLastOperation[mId] = vOps[i];

        if (op.operationId <= 0) {
            // first operation
            newJobOrder.push_back(op.jobId);
        }
    }
    auto depth = oldVertex.vertexDepth() + 1; // you are a direct child of the old vertex

    // encountered ops same as scheduled ops unless we merge
    auto encounteredOps = newScheduledOps;

    auto newVertex = std::make_shared<DD::Vertex>(vertexId++,
                                                  std::move(newMachineEdges),
                                                  std::move(ASAPST),
                                                  std::move(ALAPST),
                                                  std::move(newJobsCompletion),
                                                  std::move(newJobOrder),
                                                  std::move(newLastOperation),
                                                  std::move(newScheduledOps),
                                                  depth,
                                                  std::move(encounteredOps));
    newVertex->setReadyOperations(problemInstance, graphIsRelaxed);
    return newVertex;
}

std::vector<SharedVertex>
algorithm::DDSolver::expandVertex(std::uint64_t &vertexId,
                                  const DD::Vertex &state,
                                  DelayGraph::delayGraph &dg,
                                  const FORPFSSPSD::Instance &problemInstance) {
    std::vector<SharedVertex> expandedStates;

    for (const auto &[jId, ops] : state.readyOps()) {
        auto [newEdges, readyVIDs] = createSchedulingOptionEdges(problemInstance, dg, state, ops);

        auto newASAPST = state.getASAPST();
        auto newALAPST = state.getALAPST();

        auto inferredEdges = DDSolver::inferEdges(state, problemInstance, dg);
        inferredEdges.insert(inferredEdges.end(), newEdges.begin(), newEdges.end());

        // Check if updating the path with a new edge is feasible while also inferring lower bound
        if (!algorithm::LongestPath::addEdgesSuccessful(dg, inferredEdges, newASAPST)) {
            LOG_T("welp, infeasible {} \n", vertexId);
            continue;
        }

        DDSolver::updateVertexALAPST(newASAPST, newALAPST, dg, state.scheduledOps(), newEdges, ops);

        auto newState = DDSolver::createNewVertex(vertexId,
                                                  state,
                                                  problemInstance,
                                                  readyVIDs,
                                                  ops,
                                                  std::move(newASAPST),
                                                  std::move(newALAPST),
                                                  std::move(newEdges),
                                                  false);
        expandedStates.push_back(newState);
    }
    return expandedStates;
}

bool algorithm::DDSolver::findVertexDominance(JobIdxToVertices &activeVertices,
                                              const SharedVertex &newVertex,
                                              const FORPFSSPSD::Instance &problemInstance) {

    // this selects those active vertices that have reached the same level of job completion as the
    // new vertex being considered
    auto &possibleVertices = activeVertices[newVertex->getJobsCompletion()];

    for (const auto &[id, v] : possibleVertices) {
        if (isDominated(*newVertex, *v, problemInstance)) {
            // It is dominated we can discard it
            LOG("state is dominated");
            return true;
        }
    }

    // Iterators become invalid after removing elements from the map so we first make a list of
    // the elements to remove
    std::vector<std::size_t> toRemove;
    for (const auto &[id, v] : possibleVertices) {
        if (isDominated(*v, *newVertex, problemInstance)) {
            // It dominates the vertex so we remove it
            LOG("state dominates");
            toRemove.push_back(id);
        }
    }

    for (const auto &id : toRemove) {
        possibleVertices.erase(id);
    }

    // If it is not dominated, we add it
    LOG("state is added");
    possibleVertices.emplace(newVertex->id(), newVertex);
    return false;
};

bool algorithm::DDSolver::isDominated(const DD::Vertex &newVertex,
                                      const DD::Vertex &oldVertex,
                                      const FORPFSSPSD::Instance &problem) {
    const auto &dg = problem.getDelayGraph();
    const auto &allEdgesNew = newVertex.getMachineEdges();
    const auto &allEdgesOld = oldVertex.getMachineEdges();

    if (allEdgesNew.size() != allEdgesOld.size()) {
        return false;
    }

    const auto &newASAPST = newVertex.getASAPST();
    const auto &oldASAPST = oldVertex.getASAPST();
    const auto &newALAPST = newVertex.getALAPST();
    const auto &oldALAPST = oldVertex.getALAPST();

    bool unscheduledOpsDominance = false;
    // const auto allReadyOps = newVertex.immediatelyReadyOps();
    bool readyDone = false;
    FORPFSSPSD::OperationsVector allReadyOps;

    // If it is dominated, possible start time (machine availability + sequence dependent setup
    // times) are always higher for the old than for the new
    const bool opStartTimeDominance =
            std::all_of(allEdgesNew.begin(), allEdgesNew.end(), [&](const auto &machineEdgesNew) {
                const auto &mId = machineEdgesNew.first;
                const auto &edgesOld = allEdgesOld.at(mId);
                const auto &edgesNew = machineEdgesNew.second;
                if (machineEdgesNew.second.empty() || edgesOld.empty()) {
                    LOG(LOGGER_LEVEL::WARNING, "Empty edges for machine {}", machineEdgesNew.first);
                    return false;
                }

                if (!readyDone) {
                    allReadyOps = newVertex.immediatelyReadyOps();
                    readyDone = true;
                }

                FORPFSSPSD::OperationsVector readyOps;
                std::copy_if(allReadyOps.begin(),
                             allReadyOps.end(),
                             back_inserter(readyOps),
                             [&](const auto &op) { return problem.getMachine(op) == mId; });
                const auto vIdDstNew = edgesNew.back().dst;
                const auto vIdDstOld = edgesOld.back().dst;
                if (readyOps.empty()) {
                    return newASAPST.at(vIdDstNew) + problem.getProcessingTime(vIdDstNew)
                           >= oldASAPST.at(vIdDstOld) + problem.getProcessingTime(vIdDstOld);
                }

                const auto &opDstNew = dg.get_vertex(vIdDstNew).operation;
                const auto &opDstOld = dg.get_vertex(vIdDstOld).operation;

                return std::all_of(readyOps.begin(), readyOps.end(), [&](const auto &op) {
                    return newASAPST.at(vIdDstNew) + problem.query(opDstNew, op)
                           >= oldASAPST.at(vIdDstOld) + problem.query(opDstOld, op);
                });
            });

    if (opStartTimeDominance) {
        unscheduledOpsDominance = true;
        const auto &opsSched = newVertex.scheduledOps();
        const auto &opsReady = newVertex.immediatelyReadyOps();

        for (std::size_t vId = 0; vId < newASAPST.size() && unscheduledOpsDominance; ++vId) {
            if (std::find(opsSched.begin(), opsSched.end(), vId) != opsSched.end()) {
                // it is always dominated based on this operation because we only check yet to be
                // scheduled ops
                continue;
            }
            // if it is in ready ops then the check is already handled by opDom

            if (std::find(opsReady.begin(), opsReady.end(), dg.get_vertex(vId).operation)
                != opsReady.end()) {

                unscheduledOpsDominance =
                        (oldALAPST[vId] - oldASAPST[vId]) <= (newALAPST[vId] - newASAPST[vId]);
                continue;
            }
            unscheduledOpsDominance =
                    (oldASAPST[vId] >= newASAPST[vId])
                    && ((oldALAPST[vId] - oldASAPST[vId]) <= (newALAPST[vId] - newASAPST[vId]));
        }
    }

    return opStartTimeDominance && unscheduledOpsDominance;
}

bool algorithm::DDSolver::isTerminal(const DD::Vertex &vertex,
                                     const FORPFSSPSD::Instance &instance) {
    const auto &jobsOutput = instance.getJobsOutput();
    const auto &jobsCompletion = vertex.getJobsCompletion();
    for (std::size_t i = 0; i < jobsOutput.size(); ++i) {
        const auto &jobOps = instance.jobs(jobsOutput[i]);
        if (jobsCompletion[i] < jobOps.size()) {
            return false;
        }
    }
    return true;
}

void algorithm::DDSolver::updateVertexALAPST(const PathTimes &ASAPST,
                                             PathTimes &ALAPST,
                                             DelayGraph::delayGraph &dg,
                                             const DelayGraph::VerticesIDs &scheduledOps,
                                             const DelayGraph::Edges &newestEdges,
                                             const std::vector<FORPFSSPSD::operation> &newestOps) {
    // TO DO: Can we have a version of this function that increments based on adding one new edge?
    // Tricky because apart from adding an edge, we also update some values

    for (auto i : scheduledOps) {
        ALAPST[i] = ASAPST[i];
    }
    for (auto op : newestOps) {
        ALAPST[dg.get_vertex_id(op)] = ASAPST[dg.get_vertex_id(op)];
    }
    dg.add_edges(newestEdges);
    // Note that newestOp should bed part of scheduledOps passed to this function
    // It doesnt matter at the moment because the ASAPST check already guarntees this is efasible
    // But if it were infeasible we may lose chance to spot it
    auto newALAPresult = LongestPath::computeALAPST(dg, ALAPST, scheduledOps);
    dg.remove_edges(newestEdges);
}

SharedVertex DDSolver::popQueue(std::deque<SharedVertex> &states) {
    SharedVertex v = states.front();
    states.pop_front();
    return v;
}

template <class F>
SharedVertex DDSolver::popQueue(std::vector<SharedVertex> &states, F comparator) {
    SharedVertex v = states.front();
    std::pop_heap(states.begin(), states.end(), comparator);
    states.pop_back();
    return v;
}

template <class V>
void DDSolver::pushQueue(std::vector<SharedVertex> &states, SharedVertex &s, V comparator) {
    states.push_back(s);
    std::push_heap(states.begin(), states.end(), comparator);
}

void DDSolver::pushQueue(std::deque<SharedVertex> &states,
                         SharedVertex &s,
                         DDExplorationType comparator) {
    if (comparator == DDExplorationType::BREADTH) {
        states.push_back(s);
    } else if (comparator == DDExplorationType::DEPTH) {
        states.push_front(s);
    } else {
        throw std::runtime_error(
                "FmsScheduler::wrong graph exploration heuristic supplied with deque type");
    }
}

// template <typename T, class V>
template <class V> void DDSolver::orderQueue(std::vector<SharedVertex> &states, V comparator) {
    std::make_heap(states.begin(), states.end(), comparator);
}

template <typename T>
void DDSolver::push(T &states,
                    DDExplorationType explorationType,
                    SharedVertex &newVertex,
                    const DD::DDSolution &solution,
                    bool reOrder) {
    switch (explorationType) {
    case DDExplorationType::DEPTH:
        DDSolver::pushQueue(states, newVertex, explorationType);
        break;

    case DDExplorationType::STATIC:
    case DDExplorationType::ADAPTIVE:
        if (reOrder) {
            DDSolver::orderQueue(states, DD::CompareVerticesRanking(solution));
        }
        DDSolver::pushQueue<DD::CompareVerticesRanking>(
                states, newVertex, DD::CompareVerticesRanking(solution));
        break;
    case DDExplorationType::BREADTH:
        DDSolver::pushQueue(states, newVertex, explorationType);
        break;
    case DDExplorationType::BEST:
        if (reOrder) {
            DDSolver::orderQueue(states, DD::CompareVerticesLowerBound());
        }
        DDSolver::pushQueue<DD::CompareVerticesLowerBound>(
                states, newVertex, DD::CompareVerticesLowerBound());
        break;
    default:
        throw std::runtime_error("FmsScheduler::unknown graph exploration heuristic supplied -- "
                                 "only depth and static currently accepted for DD seed scheduler");
    }
}

template <typename T>
SharedVertex DDSolver::pop(T &states, DDExplorationType explorationType, DD::DDSolution &solution) {
    switch (explorationType) {
    case DDExplorationType::DEPTH:
    case DDExplorationType::BREADTH:
        return popQueue(states);
    case DDExplorationType::BEST:
        return popQueue(states, DD::CompareVerticesLowerBound());
    case DDExplorationType::STATIC:
    case DDExplorationType::ADAPTIVE:
        return popQueue(states, DD::CompareVerticesRanking(solution));
    default:
        throw std::runtime_error("FmsScheduler::unknown graph exploration heuristic supplied -- "
                                 "only depth and static currently accepted for DD seed scheduler");
    }
}

// Merge operator to create a relaxed diagram
template <typename T, class F>
void DDSolver::mergeLoop(T &states,
                         std::uint64_t &vertexId,
                         const FORPFSSPSD::Instance &problemInstance,
                         const DelayGraph::delayGraph &dg) {
    while (states.size() > 100) { // fix width to 100 for now
        // select vertices to merge
        // remove them from states

        std::uint32_t firstIndex = chooseVertexToMerge(states.size());
        const auto firstVertex = states[firstIndex];
        states.erase(states.begin() + firstIndex);

        std::uint32_t secondIndex = chooseVertexToMerge(states.size());
        const auto secondVertex = states[secondIndex];
        states.erase(states.begin() + secondIndex); // find element and remove it from vector

        // perform merge
        auto newVertex = mergeOperator(firstVertex, secondVertex, vertexId, problemInstance, dg);
        states.emplace_back(std::move(newVertex));
    }
}

algorithm::DDSolver::SharedVertex
DDSolver::mergeOperator(const DD::Vertex &a,
                        const DD::Vertex &b,
                        std::uint64_t &vertexId,
                        const FORPFSSPSD::Instance &problemInstance,
                        const DelayGraph::delayGraph &dg) {

    algorithm::PathTimes mergedASAPST;
    auto aASAPST = a.getASAPST();
    auto bASAPST = b.getASAPST();
    mergedASAPST.reserve(aASAPST.size());
    std::transform(aASAPST.begin(),
                   aASAPST.end(),
                   bASAPST.begin(),
                   std::back_inserter(mergedASAPST),
                   [](auto x, auto y) { return std::min(x, y); });

    algorithm::PathTimes mergedALAPST;
    auto aALAPST = a.getALAPST();
    auto bALAPST = b.getALAPST();
    mergedALAPST.reserve(aALAPST.size());
    std::transform(aALAPST.begin(),
                   aALAPST.end(),
                   bALAPST.begin(),
                   std::back_inserter(mergedALAPST),
                   [](auto x, auto y) { return std::max(x, y); });

    DD::MachineEdges mergedMachineEdges =
            {}; // merging for relaxation loses machine edges as we don't keep sequence anymore

    DD::JobIdxToOpIdx mergedJobsCompletion;
    auto aJobsCompletion = a.getJobsCompletion();
    auto bJobsCompletion = b.getJobsCompletion();
    mergedJobsCompletion.reserve(aJobsCompletion.size());
    std::transform(aJobsCompletion.begin(),
                   aJobsCompletion.end(),
                   bJobsCompletion.begin(),
                   std::back_inserter(mergedJobsCompletion),
                   [](auto x, auto y) { return std::max(x, y); });

    std::vector<FORPFSSPSD::JobId> mergedJobOrder =
            {}; // relaxation clears job order and then allows overtaking

    DelayGraph::VerticesIDs mergedScheduledOps;
    auto aScheduledOps = a.scheduledOps();
    auto bScheduledOps = b.scheduledOps();
    std::set_intersection(aScheduledOps.begin(),
                          aScheduledOps.end(),
                          bScheduledOps.begin(),
                          bScheduledOps.end(),
                          std::back_inserter(mergedScheduledOps));

    DelayGraph::VerticesIDs mergedEncounteredOps;
    auto aEncounteredOps = a.encounteredOps();
    auto bEncounteredOps = b.encounteredOps();
    std::set_union(aEncounteredOps.begin(),
                   aEncounteredOps.end(),
                   bEncounteredOps.begin(),
                   bEncounteredOps.end(),
                   std::back_inserter(mergedEncounteredOps));

    DD::MachineToVertex mergedLastOperation; // TO DO - make this more efficient than loop and check
    // for each machine, it is the maximum asapst of things already encountered on that machine
    for (auto vId : mergedEncounteredOps) {
        const auto mId = problemInstance.getMachine(dg.get_vertex(vId).operation);
        if (mergedLastOperation.find(mId) == mergedLastOperation.end()) {
            mergedLastOperation[mId] = vId;
            continue;
        }
        if (mergedASAPST[vId] > mergedASAPST[mergedLastOperation[mId]]) {
            mergedLastOperation[mId] = vId;
            continue;
        }
    }

    auto mergedDepth = mergedEncounteredOps.size();

    std::uint64_t mergedID = ++vertexId;

    auto mergedVertex = std::make_shared<DD::Vertex>(mergedID,
                                                     std::move(mergedMachineEdges),
                                                     std::move(mergedASAPST),
                                                     std::move(mergedALAPST),
                                                     std::move(mergedJobsCompletion),
                                                     std::move(mergedJobOrder),
                                                     std::move(mergedLastOperation),
                                                     std::move(mergedScheduledOps),
                                                     mergedDepth,
                                                     std::move(mergedEncounteredOps));
    mergedVertex->setReadyOperations(
            problemInstance, true); // something clumsy about always having to do this extra step
    return mergedVertex;
}

std::uint32_t algorithm::DDSolver::chooseVertexToMerge(std::uint32_t size) {
    // chooses randomly for now
    // other heuristics should be tried out
    return rand() % size;
}
