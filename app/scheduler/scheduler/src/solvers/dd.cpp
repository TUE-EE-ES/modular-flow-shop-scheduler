#include "fms/pch/containers.hpp"
#include "fms/pch/utils.hpp"
#include "fms/pch/fmt.hpp"

#include "fms/solvers/dd.hpp"

#include "fms/cg/builder.hpp"
#include "fms/cg/constraint_graph.hpp"
#include "fms/cg/edge.hpp"
#include "fms/cg/export_utilities.hpp"
#include "fms/dd/comparator.hpp"
#include "fms/dd/vertex.hpp"
#include "fms/problem/flow_shop.hpp"
#include "fms/problem/indices.hpp"
#include "fms/solvers/sequence.hpp"
#include "fms/solvers/utils.hpp"

#include <algorithm>
#include <cstring>

static constexpr float kDefaultRankFactor = 0.8;

namespace {

void updateBounds(fms::solvers::dd::DDSolverData &data) {
    const auto &states = data.states;
    const auto bestLowerBoundElement =
            std::min_element(states.begin(), states.end(), fms::dd::CompareVerticesLowerBoundMin());
    auto minLowerBound = bestLowerBoundElement == states.end()
                                 ? data.solution.bestUpperBound()
                                 : std::min((*bestLowerBoundElement)->lowerBound(),
                                            data.solution.bestUpperBound());
    fms::LOG_D("Lower is {} and upper is {}", minLowerBound, data.solution.bestUpperBound());
    data.solution.setBestLowerBound(minLowerBound);
}

} // namespace

namespace fms::solvers::dd {

void DDSolverData::storeState(const SharedVertex &newVertex) {
    if (storeAllStates) {
        allStates.push_back(newVertex);
    }
}

/* Input: Problem instance and command line arguments.
 * This function sets up and executes the scheduling algorithm based on the selected exploration
 * strategy.
 * It returns a tuple containing the solutions found and json of the data of the algorithm.
 */
std::tuple<Solutions, nlohmann::json> solve(problem::Instance &problemInstance,
                                            const cli::CLIArgs &args) {
    auto [solutions, dataJSON, _] = solveWrap(problemInstance, args, nullptr);
    return {std::move(solutions), std::move(dataJSON)};
}

/* Input: command line arguments, problem instance and oldData
 * This function sets up the initial state of the algorithm.
 * It puts the root state in the queue and sets up the delay graph.
 * If a seed solution is available, it will create the vertices for the seed solution and add them
 * to the queue. The queue always contains the vertices that are to be explored next in the
 * algorithm. The function returns a new instance of DDSolverData, encapsulating the
 * current state, the solution object, the delay graph, a flag indicating how to manage active
 * vertices, and a vertex ID counter, ready for the main solving process.
 */
DDSolverDataPtr
initialize(const cli::CLIArgs &args, problem::Instance &instance, DDSolverDataPtr dataOld) {
    if (dataOld != nullptr) {
        return std::move(dataOld);
    }

    bool storeAllStates = false;
    for (const auto &arg : args.algorithmOptions) {
        if (arg == kStoreHistory) {
            storeAllStates = true;
        }
    }

    DDSolution solution(utils::time::getCpuTime(), kDefaultRankFactor, instance.getTotalOps());

    // Generate the base graph
    auto dg = cg::Builder::jobShop(instance);
    instance.updateDelayGraph(dg);

    if (IS_LOG_D()) {
        auto name = fmt::format("input_delayGraph_{}.dot", instance.getProblemName());
        cg::exports::saveAsDot(dg, name);
    }

    auto [result, ASAPST] = SolversUtils::checkSolutionAndOutputIfFails(instance);
    auto [_, ALAPST] = algorithms::paths::computeALAPST(dg);

    LOG("Number of vertices in the delay graph is {} ", dg.getNumberOfVertices());
    LOG("ASAPST is of size {} with the last operation being ({}) started at {}.",
        ASAPST.size(),
        dg.getVertex(ASAPST.size() - 1).operation,
        ASAPST.back());

    const auto &jobs = instance.jobs();
    const auto &machines = instance.getMachines();

    // when this bool is true, then the algorithm will remove the active vertices that
    // have already been explored
    bool keepActiveVerticesSparse = true;

    auto data = std::make_unique<DDSolverData>(args.explorationType,
                                               std::move(solution),
                                               std::move(dg),
                                               keepActiveVerticesSparse,
                                               storeAllStates);
    // Initialize the root state
    // ASAPST is the earliest start time for each operation
    // ALAPST is the latest start time for each operation
    auto root = std::make_shared<Vertex>(data->nextVertexId++,
                                         0,
                                         MachinesSequences{},
                                         ASAPST,
                                         ALAPST,
                                         JobIdxToOpIdx(jobs.size(), 0),
                                         std::vector<problem::JobId>{},
                                         MachineToVertex{},
                                         cg::VerticesIds{});
    root->setReadyOperations(instance);

    // Add root to the queue. It will the be first state to be explored unless we provide a seed
    push(data->states, args.explorationType, root, solution, true);
    data->storeState(root);

    // get seed solution if available
    if (!args.sequenceFile.empty()) {
        auto seedSolution = getSeedSolution(instance, args);

        // add seed solution to solutions
        LOG("best upper bound from heuristic is {}", solution.bestUpperBound());

        // create vertices for every state of solution to continue to build from
        // add seed vertices to active vertices
        // add all children of seed vertices to states queue
        initialiseStates(instance, *data, args, seedSolution, root);
        keepActiveVerticesSparse = false;
    }

    return data;
}

bool shouldStop(const DDSolverData &data, const cli::CLIArgs &args, const std::size_t iterations) {
    const auto timeElapsed = utils::time::getCpuTime() - data.solution.start();
    return data.states.empty() || timeElapsed >= args.timeOut || iterations >= args.maxIterations
           || data.solution.isOptimal();
}

void singleIteration(DDSolverData &data, const problem::Instance &problemInstance) {
    SharedVertex s = pop(data);

    const auto &sLastOperation = s->getLastOperation();
    const auto currentDepth = s->vertexDepth();

    // The vertex becomes inactive after expansion so we remove it
    if (data.keepActiveVerticesSparse) {
        removeActiveVertex(data.activeVertices, *s);
    }

    if (isTerminal(*s, problemInstance)) {
        // if terminal we should update upper bound and add state to another queue otherwise we
        // lose it
        data.solution.addNewSolution(*s);
        if (data.solution.isOptimal()) {
            LOG("Solution is optimal");
        }
        return;
    }
    const auto addedEdges = data.dg.addEdges(s->getAllEdges(problemInstance));

    LOG("Expanding state");
    auto expandedStates = expandVertex(data, *s, problemInstance);

    // For each ready operation(s), attempt to schedule and extend to a new state
    // If dominated, discard
    // If dominates, replace and propagate release to child nodes
    for (auto newState : expandedStates) {

        // First check if it is dominated upper bound wise and discard if it is
        // Then check if it dominates another state or it is not dominated we create a new state
        // and add it. Otherwise we discard it. These checks have to happen in this order
        // otherwise findandmerge could add a vertex to active vertices and it ends up discarded
        // causing a pointer error.

        if (newState->lowerBound() > data.solution.bestUpperBound()) {
            data.storeState(newState);
            continue;
        }
        if (findVertexDominance(data.activeVertices, newState, problemInstance)) {
            // comment the following line to prune the tree
            data.storeState(newState);
            continue;
        }

        // s->addChild(newState);
        push(data, newState, false);

        data.storeState(newState);
    }

    // update best lower bound
    ::updateBounds(data);

    // Remove the edges that were added
    data.dg.removeEdges(addedEdges);

    LOG_D("Queue is {} elements long", data.states.size());

    if (data.solution.isOptimal()) {
        LOG("Solution is optimal");
        return;
    }

    if (data.states.empty()) {
        LOG("States is empty");
    }
}

/*Input: a problem instance, command line arguments and oldData
 * It initialises the solver state with possible previous data to resume from
 *
 * Has a while loop that processes until shouldstop (the stopping condition) is reached and does the
 * following things:
 *
 * extracts shared vertex, removes the active vertex if keepactiveverticessparce is true
 * checks if s is a terminal state in the decision diagram, if it is we update the solution and
 * check for optimality after adding the new solution
 *
 * Expand the current vertex and update the graph with its edges
 * Add each new state generated by the current vertex and discard states that aren't feasable or not
 * dominated by better states. Add a new state to the solver state
 *
 * Then update global bounds based on the latest states processed and remove the temporarily added
 * vertices after expansion.
 *
 * After the while loop, we process a json file (storing the solution)
 */
ResumableSolverOutput
solveWrap(problem::Instance &problemInstance, const cli::CLIArgs &args, DDSolverDataPtr oldData) {

    LOG("Computation of the schedule using DD started");
    auto data = initialize(args, problemInstance, std::move(oldData));

    // Count iterations
    std::uint32_t iterations = 0;

    while (!shouldStop(*data, args, iterations)) {
        singleIteration(*data, problemInstance);
        ++iterations;
    }

    return solveTerminate(std::move(data));
}

ResumableSolverOutput solveTerminate(DDSolverDataPtr data) {
    auto dataJSON = data->solution.getSolveData();

    // Find termination reason
    if (data->solution.isOptimal()) {
        dataJSON["terminationReason"] = TerminationStrings::kOptimal;
    } else if (data->states.empty()) {
        dataJSON["terminationReason"] = TerminationStrings::kNoSolution;
    } else {
        dataJSON["terminationReason"] = TerminationStrings::kTimeOut;
        // The other two reasons already print a message inside the loop
        LOG("DD: Time out");
    }

    const auto &solutions = data->solution.getStatesTerminated();
    return {extractSolutions(solutions), std::move(dataJSON), std::move(data)};
}

/*
 * Input: problem instance and command line arguments
 * Returns the partial solution object that serves as a starting point for further optimization
 */
PartialSolution getSeedSolution(problem::Instance &problemInstance, const cli::CLIArgs &args) {
    return std::get<0>(sequence::solve(problemInstance, args)).front();
}

/*
 * Input: problem instance, delaygraph, oldvertex and ops (list of operations)
 * oldvertex is the vertex from which new scheduling operations are derived
 *
 * reserve memory based on the number of operations with the reserve command
 *
 * We retrieve the last operation processed by each machine from the oldvertex, and we loop
 * through each operation to create new edges in the delay graph. In this for loop we identify the
 * machine that handles the operation, and retrieve the corresponding vertex from the delay graph.
 * We find the last operation that was performed on the same machine, if no such operation exists,
 * we use the source vertex for that machine as the starting point and then we create a new edge
 * from the last operation vertex to the current operation vertex We add the current vertex id to
 * the list of where the vertex id's are.
 *
 * We return the newly created edges and the list of ready vertex ids
 */
std::tuple<cg::Edges, cg::VerticesIds>
createSchedulingOptionEdges(const problem::Instance &problemInstance,
                            const cg::ConstraintGraph &dg,
                            const Vertex &oldVertex,
                            const std::vector<problem::Operation> &ops) {
    cg::Edges newEdges{};
    cg::VerticesIds readyVIDs{};

    newEdges.reserve(ops.size());
    readyVIDs.reserve(ops.size());

    const auto &stateLastOperation = oldVertex.getLastOperation();
    for (auto op : ops) {
        const auto mId = problemInstance.getMachine(op);
        const auto &v = dg.getVertex(op);

        const auto &lastOp = stateLastOperation.find(mId);

        const auto &lastVertex = lastOp == stateLastOperation.end() ? dg.getSource(mId)
                                                                    : dg.getVertex(lastOp->second);

        newEdges.emplace_back(lastVertex.id, v.id, problemInstance.query(lastVertex, v));
        readyVIDs.push_back(v.id);
    }
    return {std::move(newEdges), std::move(readyVIDs)};
}

/*
 * Input: problem instance, command line arguments, delaygraph, seedsolution (which is a partial
 * solution and has been computed earlier) vertexid (uint64), oldvertex and solution
 *
 * vertexid is a reference to a counter used for assigning unique id's to new vertices.
 *
 * We retrieve the selected edges in seedsolution, organised by machine
 * Check the type of shop (this is an enumeration type), in our case we use the flowshop type
 *
 * We extract the jobid from the seed solution edges, and then each edge leads to a job
 * these jobs are collected into a jobordervector. The getjoborder is used together with joborder to
 * initialise states specific to a flowshop scenario
 *
 * For other shop types that aren't flowshop, the order is not that relevant.
 * Initialize states using the machine order.
 */
void initialiseStates(const problem::Instance &problem,
                      DDSolverData &data,
                      const cli::CLIArgs &args,
                      PartialSolution seedSolution,
                      SharedVertex &rootVertex) {
    const auto &seedSolutionSequence = seedSolution.getChosenSequencesPerMachine();

    SharedVertex oldVertex = rootVertex;

    // Sort seed solution edges by machine index
    std::vector<problem::MachineId> machineOrder;
    machineOrder.reserve(seedSolutionSequence.size());
    for (const auto &it : seedSolutionSequence) {
        machineOrder.push_back(it.first);
    }
    std::sort(machineOrder.begin(), machineOrder.end());

    for (const auto &mId : machineOrder) {
        std::reference_wrapper<const cg::Vertex> vPrevious = data.dg.getSource(mId);
        for (const auto &op : seedSolutionSequence.at(mId)) {
            const auto vOp = data.dg.getVertex(op);
            cg::Edge edge(vPrevious.get().id, vOp.id, problem.query(vPrevious.get(), vOp));

            auto stateASAPST = oldVertex->getASAPST();
            auto stateALAPST = oldVertex->getALAPST();

            auto inferredEdges = inferEdges(*oldVertex, problem, data.dg);
            inferredEdges.push_back(edge);

            // Check whether updating the path with a new edge is feasible while also inferring
            // lower bound
            if (algorithms::paths::addEdgesIncrementalASAPST(data.dg, inferredEdges, stateASAPST)) {
                // No, adding one edge creates a cycle - simultaneously updates ASAPST
                // Throw error because seed solution should always be feasible
                throw FmsSchedulerException("The seed solution is infeasible");
            }

            updateVertexALAPST(
                    stateASAPST, stateALAPST, data.dg, oldVertex->scheduledOps(), {edge}, {op});

            auto newVertex = createNewVertex(data.nextVertexId,
                                             *oldVertex,
                                             problem,
                                             {vOp.id},
                                             {op},
                                             std::move(stateASAPST),
                                             std::move(stateALAPST));

            push(data.states, args.explorationType, newVertex, data.solution, true);
            data.storeState(newVertex);
            oldVertex = newVertex;
        }
    }

    if (!isTerminal(*oldVertex, problem)) {
        throw FmsSchedulerException("Seed solution is not terminal");
    }

    data.solution.addNewSolution(*oldVertex);
}

/*
 *
 * input: Jobidxtovertices, vertex
 * We search for a vertex in activevertices that matches the vertexjobcompletionstatus
 * If vertex v is not found, then the function just returns because there is nothing to remove
 * Else it removes that vertex
 */
void removeActiveVertex(JobIdxToVertices &activeVertices, const Vertex &v) {
    auto it = activeVertices.find(v.getJobsCompletion());
    if (it == activeVertices.end()) {
        return;
    }

    it->second.erase(v.id());
    if (it->second.empty()) {
        activeVertices.erase(it);
    }
}

/*
 * Input: all vertices that have been terminated
 *
 * For each vertex in the terminated vertices, we create a new solution and in that solution we
 * iterate over all machine edges. For each edge in the machine edges we just log it.
 * We also log how many solutions were found and return all solutions
 */
Solutions extractSolutions(const std::vector<Vertex> &statesTerminated) {
    Solutions solutions;
    for (const auto &state : statesTerminated) {
        LOG("New Solution");
        for (const auto &[m, sequence] : state.getMachinesSequences()) {
            LOG(fmt::format(FMT_COMPILE("Machine {} has the following sequence"), m));
            for (auto op : sequence) {
                LOG(fmt::to_string(op));
            }
        }
        PartialSolution solution(state.getMachinesSequences(), state.getASAPST());
        solutions.push_back(solution);
    }
    LOG(FMT_COMPILE("Found {} solutions"), solutions.size());
    return solutions;
}

/*
 * Infers additional edges in the delay graph based on the current vertex state and the overall
 * problem instance. This method aims to generate edges that represent scheduling decisions still
 * pending in the solution process, focusing on operations that haven't been scheduled yet.
 *
 * input: s, problem instance and dg
 *  s is the current state of the vertex from which new edges are inferred.
 *  dg is the current state of the delay graph, which is being dynamically updated as the algorithm
 * progresses.
 *
 * @return A set of inferred edges which might not exist explicitly in the original problem but are
 * necessary for completing the scheduling logic. These edges help in computing the earliest and
 * latest start times of operations by considering unscheduled operations and their constraints.
 */
cg::Edges
inferEdges(const Vertex &s, const problem::Instance &problem, const cg::ConstraintGraph &dg) {
    cg::Edges inferredEdges;

    const auto &lastOperation = s.getLastOperation();
    const auto &scheduledOps = s.scheduledOps();
    const std::unordered_set<cg::VertexId> scheduledOpsFast(scheduledOps.begin(),
                                                            scheduledOps.end());

    // trying total processing time for an even tighter bound
    // TO DO: keep this value in the state and subtract everytime we make a new sched decision
    std::unordered_map<problem::MachineId, delay> machineTotalTimeLeft;
    for (const auto &[opTo, mId] : problem.machineMapping()) {
        const auto vIdTo = dg.getVertexId(opTo);

        if (scheduledOpsFast.contains(vIdTo)) {
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
            inferredEdges.emplace_back(dg.getSource(mId).id, vIdTo, 0);
        } else {
            inferredEdges.emplace_back(it2->second, vIdTo, problem.getProcessingTime(it2->second));
        }
    }

    // NOTE: Do we need this?
    auto terminalV = dg.getTerminus();
    for (auto [mId, time] : machineTotalTimeLeft) {
        const auto it2 = lastOperation.find(mId);
        // Insert it from the source if not found
        cg::VertexId vIdFrom = it2 == lastOperation.end() ? dg.getSource(mId).id : it2->second;
        inferredEdges.emplace_back(vIdFrom, terminalV.id, time);
    }
    return inferredEdges;
}

/*
 * Input: vertexid, oldvertex, probleminstance, delaygraph, ops (operation vector),
 * pathtimes as soon as possible start times, as late as possible start times, delaygraph and
 * graphIsRelaxed
 *
 * We get the new values for joborder, getjobscompletion, getmachineedges, scheduledops and
 * lastoperation, and we create a new vertex representing a state in the decision diagram for the
 * scheduling problem. This function constructs a vertex encapsulating the scheduling decisions made
 * thus far, including operations scheduled, the sequence of jobs, and their earliest and latest
 * start times.
 *
 *
 * We iterate through the list of operations provided for the old vertex (ops):
 * For each operation, update the machine edges to include new dependencies,
 * scheduled operations, and the last operation performed on each machine.
 * Update job completion states based on the operations being scheduled.
 * Adjust the job order if the operation is the start of a new job.
 * Set the depth of the new vertex to indicate its level in the decision diagram.
 * Initialize the new vertex with all the updated attributes and marks it with the
 * specified depth and whether operations are set as ready based on the relaxed state of the graph.
 *
 */
SharedVertex createNewVertex(std::uint64_t &nextVertexId,
                             const Vertex &oldVertex,
                             const problem::Instance &problemInstance,
                             const cg::VerticesIds &vOps,
                             const std::vector<problem::Operation> &ops,
                             algorithms::paths::PathTimes ASAPST,
                             algorithms::paths::PathTimes ALAPST,
                             const bool graphIsRelaxed) {

    auto newJobOrder = oldVertex.getJobOrder();
    auto newJobsCompletion = oldVertex.getJobsCompletion();
    auto newMachinesSequences = oldVertex.getMachinesSequences();
    auto newScheduledOps = oldVertex.scheduledOps();
    auto newLastOperation = oldVertex.getLastOperation();

    for (std::size_t i = 0; i < ops.size(); i++) {
        const auto &op = ops[i];
        const auto &mId = problemInstance.getMachine(op);
        newMachinesSequences[mId].push_back(op);
        newScheduledOps.push_back(vOps[i]);
        newLastOperation[mId] = vOps[i];

        const auto jobOutOrder = problemInstance.getJobOutputPosition(op.jobId);
        newJobsCompletion[jobOutOrder] += 1;

        if (op.operationId <= 0) {
            // first operation
            newJobOrder.push_back(op.jobId);
        }
    }
    auto depth = oldVertex.vertexDepth() + 1; // you are a direct child of the old vertex

    // encountered ops same as scheduled ops unless we merge
    auto encounteredOps = newScheduledOps;

    auto newVertex = std::make_shared<Vertex>(nextVertexId++,
                                              oldVertex.id(),
                                              std::move(newMachinesSequences),
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

/*
 * Input: data, state (queue of vertices to be explored), probleminstance and expandedstates
 * It iterates over each set of the ready operations in the current state.
 * It generates new potential edges for the scheduling graph based on the current ready operations.
 * We duplicate the asap and alap scheduling times to update them for a new state, and then
 * we check if adding these edges to the graph is feasible without creating cycles.
 * Then we update the alap times based on the new edges and the current set of scheduled operations.
 * We create a new vertex representing the state derived from the current state with the newly
 * inferred schedule. Then we add the newly created state to the list of expanded states. This
 * method returns all newly created states. (expanded states)
 */

std::vector<SharedVertex>
expandVertex(auto &data, const Vertex &state, const problem::Instance &problemInstance) {
    std::vector<SharedVertex> expandedStates;

    for (const auto &[jId, ops] : state.readyOps()) {
        auto [newEdges, readyVIDs] =
                createSchedulingOptionEdges(problemInstance, data.dg, state, ops);

        auto newASAPST = state.getASAPST();
        auto newALAPST = state.getALAPST();

        auto inferredEdges = inferEdges(state, problemInstance, data.dg);
        inferredEdges.insert(inferredEdges.end(), newEdges.begin(), newEdges.end());

        // Check if updating the path with a new edge is feasible while also inferring lower bound
        if (!algorithms::paths::addEdgesSuccessful(data.dg, inferredEdges, newASAPST)) {
            LOG_T("welp, infeasible {} \n", data.nextVertexId);
            continue;
        }

        updateVertexALAPST(newASAPST, newALAPST, data.dg, state.scheduledOps(), newEdges, ops);

        auto newState = createNewVertex(data.nextVertexId,
                                        state,
                                        problemInstance,
                                        readyVIDs,
                                        ops,
                                        std::move(newASAPST),
                                        std::move(newALAPST),
                                        false);
        expandedStates.push_back(newState);
    }
    return expandedStates;
}

bool findVertexDominance(JobIdxToVertices &activeVertices,
                         const SharedVertex &newVertex,
                         const problem::Instance &problemInstance) {

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

bool isDominated(const Vertex &newVertex,
                 const Vertex &oldVertex,
                 const problem::Instance &problem) {
    const auto &dg = problem.getDelayGraph();
    const auto &allSequencesNew = newVertex.getMachinesSequences();
    const auto &allSequencesOld = oldVertex.getMachinesSequences();

    if (allSequencesNew.size() != allSequencesOld.size()) {
        return false;
    }

    const auto &newASAPST = newVertex.getASAPST();
    const auto &oldASAPST = oldVertex.getASAPST();
    const auto &newALAPST = newVertex.getALAPST();
    const auto &oldALAPST = oldVertex.getALAPST();

    bool unscheduledOpsDominance = false;
    bool readyDone = false;
    problem::OperationsVector allReadyOps;

    // If it is dominated, possible start time (machine availability + sequence dependent setup
    // times) are always higher for the old than for the new
    const bool opStartTimeDominance = std::all_of(
            allSequencesNew.begin(), allSequencesNew.end(), [&](const auto &machineSequenceNew) {
                const auto &mId = machineSequenceNew.first;
                const auto &sequenceOld = allSequencesOld.at(mId);
                const auto &sequenceNew = machineSequenceNew.second;
                if (machineSequenceNew.second.empty() || sequenceOld.empty()) {
                    LOG_W("Empty edges for machine {}", machineSequenceNew.first);
                    return false;
                }

                // We cache this value because we only need to compute it once
                if (!readyDone) {
                    allReadyOps = newVertex.immediatelyReadyOps();
                    readyDone = true;
                }

                problem::OperationsVector readyOps;
                std::copy_if(allReadyOps.begin(),
                             allReadyOps.end(),
                             back_inserter(readyOps),
                             [&](const auto &op) { return problem.getMachine(op) == mId; });

                const auto vIdDstNew = dg.getVertexId(sequenceNew.back());
                const auto vIdDstOld = dg.getVertexId(sequenceOld.back());
                if (readyOps.empty()) {
                    return newASAPST.at(vIdDstNew) + problem.getProcessingTime(vIdDstNew)
                           >= oldASAPST.at(vIdDstOld) + problem.getProcessingTime(vIdDstOld);
                }

                const auto &opDstNew = dg.getVertex(vIdDstNew).operation;
                const auto &opDstOld = dg.getVertex(vIdDstOld).operation;

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

            if (std::find(opsReady.begin(), opsReady.end(), dg.getVertex(vId).operation)
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

bool isTerminal(const Vertex &vertex, const problem::Instance &instance) {
    const auto &jobsOutput = instance.getJobsOutput();
    const auto &jobsCompletion = vertex.getJobsCompletion();
    for (std::size_t i = 0; i < jobsOutput.size(); ++i) {
        if (jobsCompletion[i] < instance.jobs(jobsOutput[i]).size()) {
            return false;
        }
    }
    return true;
}

void updateVertexALAPST(const algorithms::paths::PathTimes &ASAPST,
                        algorithms::paths::PathTimes &ALAPST,
                        cg::ConstraintGraph &dg,
                        const cg::VerticesIds &scheduledOps,
                        const cg::Edges &newestEdges,
                        const std::vector<problem::Operation> &newestOps) {
    // TO DO: Can we have a version of this function that increments based on adding one new edge?
    // Tricky because apart from adding an edge, we also update some values

    for (auto i : scheduledOps) {
        ALAPST[i] = ASAPST[i];
    }
    for (auto op : newestOps) {
        ALAPST[dg.getVertexId(op)] = ASAPST[dg.getVertexId(op)];
    }
    dg.addEdges(newestEdges);
    // Note that newestOp should bed part of scheduledOps passed to this function
    // It doesnt matter at the moment because the ASAPST check already guarntees this is efasible
    // But if it were infeasible we may lose chance to spot it
    auto newALAPresult = algorithms::paths::computeALAPST(dg, ALAPST, scheduledOps);
    dg.removeEdges(newestEdges);
}

void push(auto &states,
          cli::DDExplorationType explorationType,
          SharedVertex &newVertex,
          const DDSolution &solution,
          bool reOrder) {
    switch (explorationType) {
    case cli::DDExplorationType::DEPTH:
        states.push_front(newVertex);
        break;

    case cli::DDExplorationType::STATIC:
    case cli::DDExplorationType::ADAPTIVE:
        if (reOrder) {
            orderQueue(states, CompareVerticesRanking(solution));
        }
        states.push_back(newVertex);
        std::push_heap(states.begin(), states.end(), CompareVerticesRanking(solution));
        break;
    case cli::DDExplorationType::BREADTH:
        states.push_back(newVertex);
        break;
    case cli::DDExplorationType::BEST:
        if (reOrder) {
            orderQueue(states, CompareVerticesLowerBound());
        }
        states.push_back(newVertex);
        std::push_heap(states.begin(), states.end(), CompareVerticesLowerBound());
        break;
    default:
        throw std::runtime_error("FmsScheduler::unknown graph exploration heuristic supplied -- "
                                 "only depth and static currently accepted for DD seed scheduler");
    }
}

SharedVertex pop(DDSolverData &data) {
    auto v = data.states.front();
    switch (data.explorationType) {
    case cli::DDExplorationType::DEPTH:
    case cli::DDExplorationType::BREADTH:
        data.states.pop_front();
        break;
    case cli::DDExplorationType::BEST:
        std::pop_heap(data.states.begin(), data.states.end(), CompareVerticesLowerBound());
        data.states.pop_back();
        break;
    case cli::DDExplorationType::STATIC:
    case cli::DDExplorationType::ADAPTIVE:
        std::pop_heap(
                data.states.begin(), data.states.end(), CompareVerticesRanking(data.solution));
        data.states.pop_back();
        break;
    default:
        throw std::runtime_error("FmsScheduler::unknown graph exploration heuristic supplied -- "
                                 "only depth and static currently accepted for DD seed scheduler");
    }

    return v;
}

// Merge operator to create a relaxed diagram
template <typename T, class F>
void mergeLoop(T &states,
               std::uint64_t &vertexId,
               const problem::Instance &problemInstance,
               const cg::ConstraintGraph &dg) {
    constexpr std::uint32_t MAX_WIDTH = 100;
    while (states.size() > MAX_WIDTH) { // fix width to 100 for now
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

SharedVertex mergeOperator(const Vertex &a,
                           const Vertex &b,
                           std::uint64_t &vertexId,
                           const problem::Instance &problemInstance,
                           const cg::ConstraintGraph &dg) {

    algorithms::paths::PathTimes mergedASAPST;
    auto aASAPST = a.getASAPST();
    auto bASAPST = b.getASAPST();
    mergedASAPST.reserve(aASAPST.size());
    std::transform(aASAPST.begin(),
                   aASAPST.end(),
                   bASAPST.begin(),
                   std::back_inserter(mergedASAPST),
                   [](auto x, auto y) { return std::min(x, y); });

    algorithms::paths::PathTimes mergedALAPST;
    auto aALAPST = a.getALAPST();
    auto bALAPST = b.getALAPST();
    mergedALAPST.reserve(aALAPST.size());
    std::transform(aALAPST.begin(),
                   aALAPST.end(),
                   bALAPST.begin(),
                   std::back_inserter(mergedALAPST),
                   [](auto x, auto y) { return std::max(x, y); });

    MachinesSequences mergedMachinesSequences =
            {}; // merging for relaxation loses machine edges as we don't keep sequence anymore

    JobIdxToOpIdx mergedJobsCompletion;
    auto aJobsCompletion = a.getJobsCompletion();
    auto bJobsCompletion = b.getJobsCompletion();
    mergedJobsCompletion.reserve(aJobsCompletion.size());
    std::transform(aJobsCompletion.begin(),
                   aJobsCompletion.end(),
                   bJobsCompletion.begin(),
                   std::back_inserter(mergedJobsCompletion),
                   [](auto x, auto y) { return std::max(x, y); });

    std::vector<problem::JobId> mergedJobOrder =
            {}; // relaxation clears job order and then allows overtaking

    cg::VerticesIds mergedScheduledOps;
    auto aScheduledOps = a.scheduledOps();
    auto bScheduledOps = b.scheduledOps();
    std::set_intersection(aScheduledOps.begin(),
                          aScheduledOps.end(),
                          bScheduledOps.begin(),
                          bScheduledOps.end(),
                          std::back_inserter(mergedScheduledOps));

    cg::VerticesIds mergedEncounteredOps;
    auto aEncounteredOps = a.encounteredOps();
    auto bEncounteredOps = b.encounteredOps();
    std::set_union(aEncounteredOps.begin(),
                   aEncounteredOps.end(),
                   bEncounteredOps.begin(),
                   bEncounteredOps.end(),
                   std::back_inserter(mergedEncounteredOps));

    MachineToVertex mergedLastOperation; // TO DO - make this more efficient than loop and check
    // for each machine, it is the maximum asapst of things already encountered on that machine
    for (auto vId : mergedEncounteredOps) {
        const auto mId = problemInstance.getMachine(dg.getVertex(vId).operation);
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

    auto mergedVertex = std::make_shared<Vertex>(mergedID,
                                                 a.parentId(),
                                                 std::move(mergedMachinesSequences),
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

std::uint32_t chooseVertexToMerge(std::uint32_t size) {
    // chooses randomly for now
    // other heuristics should be tried out
    // return rand() % size;
    return 0;
}

solvers::ResumableSolverOutput solveResumable(problem::Instance &problemInstance,
                                              problem::ProblemUpdate problemUpdate,
                                              const cli::CLIArgs &args,
                                              SolverDataPtr solverData) {

    auto dataPtr = castSolverData<DDSolverData>(std::move(solverData));
    return solveWrap(problemInstance, args, std::move(dataPtr));
}

} // namespace fms::solvers::dd
