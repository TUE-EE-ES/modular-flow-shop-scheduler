#include "pch/containers.hpp"

#include "solvers/iteratedgreedy.h"

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "FORPFSSPSD/indices.hpp"
#include "delayGraph/builder.hpp"
#include "delayGraph/delayGraph.h"
#include "longest_path.h"
#include "solvers/utils.hpp"

#include <chrono>
#include <cstdlib> /* srand, rand */
#include <fmt/chrono.h>
#include <fmt/compile.h>
#include <random>

using namespace algorithm;
using namespace std;
using namespace FORPFSSPSD;
using namespace DelayGraph;
using a_clock = std::chrono::steady_clock;

NoFixedOrderSolution IteratedGreedy::solve(FORPFSSPSD::Instance &problemInstance,
                                           const commandLineArgs &args) {
    // solve the instance
    LOG("Computation of the schedule started");

    a_clock::time_point start = a_clock::now();
    // We only support a single re-entrant machine in the system so choose the first one
    MachineId reentrant_machine = problemInstance.getReEntrantMachines().front();
    if (problemInstance.getMachineOperations(reentrant_machine).size() > 2) {
        throw std::runtime_error("Multiple re-entrancies not implemented yet");
    }

    NoFixedOrderSolution initialSolution;
    IteratedGreedy::createInitialSolution(problemInstance, reentrant_machine, initialSolution);
    // make a copy of the delaygraph
    delayGraph dg = initialSolution.getDelayGraph();
    for (auto edge : initialSolution.solution.getChosenEdges(reentrant_machine)) {
        std::cout << fmt::to_string(dg.get_vertex(edge.src)) << " " << fmt::to_string(edge) << " " << fmt::to_string(dg.get_vertex(edge.dst)) << " " << "\n";
    }

    NoFixedOrderSolution bestSolution = initialSolution;
    NoFixedOrderSolution currentSolution = initialSolution;
    
    // iterated greedy loop
    a_clock::time_point end = a_clock::now();
    while (std::chrono::duration_cast<std::chrono::microseconds>(end - start)
           < (args.timeOut * problemInstance.jobs().size())) {
        // Declare Mutator
        Mutator mutator(problemInstance, currentSolution);
        // destruction construction mutator
        mutator.DestructionConstructionMutator();
        currentSolution = mutator.output;

        // search mutations
        auto chosenMutator =
                mutator.searchMutators[rand() % mutator.searchMutators.size()]; // randomly pick a
                                                                                // search mutator
                                                                                // function to apply
        //TO DO: validate mutated solution before assigning
        (mutator.*chosenMutator)();
        auto temporarySolution = mutator.output;
        auto ASAPST = algorithm::LongestPath::initializeASAPST(dg);
        PartialSolution solution({{reentrant_machine, temporarySolution.solution.getChosenEdges(reentrant_machine)}}, {ASAPST});
        auto final_sequence = problemInstance.createFinalSequence(solution);
        auto[result,ASAPSTr] = algorithm::LongestPath::computeASAPST(dg, final_sequence);
        solution.setASAPST(ASAPSTr);

        if (result.positiveCycle.empty()){
            currentSolution = temporarySolution; // apply and update solution
        }
       
        if (currentSolution.solution.getMakespan() <= bestSolution.solution.getMakespan()) {
            std::cout << "updating best solution \n";
            bestSolution = currentSolution;
        }
        end = a_clock::now();
    }
    delayGraph bdg = bestSolution.getDelayGraph();
    for (auto edge : bestSolution.solution.getChosenEdges(reentrant_machine)) {
        std::cout << fmt::to_string(bdg.get_vertex(edge.src)) << " " << fmt::to_string(edge) << " " << fmt::to_string(bdg.get_vertex(edge.dst)) << " " << "\n";
    }
    return bestSolution;
}

void IteratedGreedy::createInitialSolution(FORPFSSPSD::Instance &problemInstance,
                                                        FORPFSSPSD::MachineId reEntrantMachine, NoFixedOrderSolution &initialSolution) {

    //randomly initialise job order
    auto jobOrder = problemInstance.getJobsOutput();
    std::shuffle(jobOrder.begin(), jobOrder.end(), std::mt19937(std::random_device()()));
    initialSolution.jobOrder = jobOrder;

    // TO DO: Modify this to check if a given permutation is feasible not the natural fixed order
    // Keep shuffling permutation until we find one that passes
    auto [result, ASAPST] = SolversUtils::checkSolutionAndOutputIfFails(problemInstance);

    DelayGraph::Edges initialSequence;
    problemInstance.updateDelayGraph(DelayGraph::Builder::FORPFSSPSD(problemInstance));
    const auto &dg = problemInstance.getDelayGraph();
    ReEntrantId reEntrantMachineId = problemInstance.findMachineReEntrantId(reEntrantMachine);

    // check how many operations are mapped
    const std::vector<OperationId> &ops = problemInstance.getMachineOperations(reEntrantMachine);

    /// no ordering is required if the number of operations mapped is 1
    if (ops.size() <= 1) {
        throw std::runtime_error(fmt::format("Machine {} is not re-entrant", reEntrantMachine));
    }

    std::optional<JobId> lastDuplexJob;

    // Add all first passes (of _duplex_ jobs) directly followed by their second passes to the
    // initial sequence
    for (auto job : problemInstance.getJobsOutput()) { // For all but the last job
        if (problemInstance.getPlexity(job, reEntrantMachineId) == FORPFSSPSD::Plexity::DUPLEX) {
            const auto &vTo = dg.get_vertex({job, ops[0]});
            const auto &vToSecond = dg.get_vertex({job, ops[1]});
            std::optional<DelayGraph::edge> edgeFirst;
            std::optional<DelayGraph::edge> edgeSecond;

            // True on first iteration
            if (!lastDuplexJob.has_value()) {
                const auto &vFrom = dg.get_source(problemInstance.getMachine(ops[0]));
                edgeFirst = DelayGraph::edge(vFrom.id, vTo.id, 0);
                edgeSecond = DelayGraph::edge(
                        vTo.id, vToSecond.id, problemInstance.query(vTo, vToSecond));
            } else {
                const auto &vFrom = dg.get_vertex({lastDuplexJob.value(), ops[1]});
                edgeFirst = DelayGraph::edge(vFrom.id, vTo.id, problemInstance.query(vFrom, vTo));
                edgeSecond = DelayGraph::edge(
                        vTo.id, vToSecond.id, problemInstance.query(vTo, vToSecond));
            }

            initialSequence.push_back(*edgeFirst);
            initialSequence.push_back(*edgeSecond);

            lastDuplexJob = job;
        }
    }

    if (!lastDuplexJob.has_value()) {
        throw FmsSchedulerException("Nothing to schedule; only simplex sheets!");
    }

    for (auto edge : initialSequence) {
        std::cout << fmt::to_string(dg.get_vertex(edge.src)) << " " << fmt::to_string(edge) << " " << fmt::to_string(dg.get_vertex(edge.dst)) << " " << "\n";
    }

    //update partial solution
    PartialSolution solution({{reEntrantMachine, initialSequence}}, ASAPST);
    initialSolution.solution = solution;
    initialSolution.jobOrder = jobOrder;
}
