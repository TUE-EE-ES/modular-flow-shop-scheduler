#include "fms/pch/containers.hpp"
#include "fms/pch/fmt.hpp"

#include "fms/solvers/iterated_greedy.hpp"

#include "fms/algorithms/longest_path.hpp"
#include "fms/cg/builder.hpp"
#include "fms/cg/constraint_graph.hpp"
#include "fms/problem/flow_shop.hpp"
#include "fms/problem/indices.hpp"
#include "fms/solvers/utils.hpp"

#include <chrono>
#include <cstdlib> /* srand, rand */
#include <random>

using namespace fms;
using namespace fms::solvers;
using a_clock = std::chrono::steady_clock;

void fms::solvers::Mutator::GapIncreaseMutator() {
    std::cout << "Executing gap increase mutator \n";
    // move all higher passes further down by one step
    cg::ConstraintGraph dg = input.getDelayGraph();
    problem::MachineId reentrant_machine = problemInstance.getReEntrantMachines().front();

    // TODO: getChosenEdges does a map search which is slow every time. Use reference wrapper?
    auto solution = input.solution;
    auto iter = solution.getMachineSequence(reentrant_machine).end();
    iter--;
    while (iter != solution.getMachineSequence(reentrant_machine).begin()) {

        auto nextOp = *iter;
        auto curOp = *std::prev(iter, 1);
        auto prevOp = *std::prev(iter, 2);

        if (prevOp.operationId < curOp.operationId) {
            // remove src from current location
            auto removalPosition =
                    std::distance(solution.getMachineSequence(reentrant_machine).begin(), iter);

            SchedulingOption rem_opt(prevOp, curOp, nextOp, removalPosition);
            solution = solution.remove(reentrant_machine,
                                       rem_opt,
                                       solution.getASAPST(),
                                       true); // making last argument true by default means
                                              // that last inserted edge is unchanged
            LOG(fmt::format("Removed {} before {}.\n", curOp, nextOp));
            for (auto op : solution.getMachineSequence(reentrant_machine)) {
                std::cout << fmt::to_string(op) << " " << "\n";
            }
            iter = solution.getMachineSequence(reentrant_machine).begin() + removalPosition;

            nextOp = *iter;
            prevOp = *std::prev(iter, 1);

            // create the option and add it
            SchedulingOption loop_opt(
                    prevOp,
                    curOp,
                    nextOp,
                    std::distance(solution.getMachineSequence(reentrant_machine).begin(), iter),
                    false);
            solution = solution.add(reentrant_machine, loop_opt, solution.getASAPST());
            LOG(fmt::format("Added {} between {} and {}.\n", curOp, prevOp, nextOp));
            iter = solution.getMachineSequence(reentrant_machine).begin() + removalPosition - 1;

            for (auto op : solution.getMachineSequence(reentrant_machine)) {
                std::cout << fmt::to_string(op) << " " << "\n";
            }
        } else {
            iter--;
        }
    }
    output = NoFixedOrderSolution{input.jobOrder, std::move(solution)};
    output.updateDelayGraph(dg);
}

void fms::solvers::Mutator::GapDecreaseMutator() {
    std::cout << "Executing gap decrease mutator \n";
    // move all higher passes further down by one step
    cg::ConstraintGraph dg = input.getDelayGraph();
    problem::MachineId reentrant_machine = problemInstance.getReEntrantMachines().front();

    // TO DO: getChosenEdges does a map search which is slow every time. Use reference wrapper?
    auto solution = input.solution;
    auto iter = solution.getMachineSequence(reentrant_machine).end();
    iter--;
    iter--;

    while (iter != solution.getMachineSequence(reentrant_machine).begin()) {

        auto nextOp = *iter;
        auto curOp = *std::prev(iter, 1);
        auto prevOp = *std::prev(iter, 2);

        if (nextOp.operationId > curOp.operationId) {
            // remove src from current location
            auto removalPosition =
                    std::distance(solution.getMachineSequence(reentrant_machine).begin(), iter);

            // because we remove the destination
            SchedulingOption rem_opt(prevOp, curOp, nextOp, removalPosition + 1);
            solution = solution.remove(reentrant_machine,
                                       rem_opt,
                                       solution.getASAPST(),
                                       true); // making last argument true by default means
                                              // that last inserted edge is unchanged
            LOG(fmt::format("Removed {} between {} and {}.\n", curOp, prevOp, nextOp));
            for (auto op : solution.getMachineSequence(reentrant_machine)) {
                std::cout << fmt::to_string(op) << " " << "\n";
            }

            // because we remove the destination
            iter = solution.getMachineSequence(reentrant_machine).begin() + removalPosition - 1;

            // insert src after dst
            nextOp = *iter;
            prevOp = *std::prev(iter, 1);

            // create the option and add it
            SchedulingOption loop_opt(
                    prevOp,
                    curOp,
                    nextOp,
                    std::distance(solution.getMachineSequence(reentrant_machine).begin(), iter),
                    false);
            solution = solution.add(reentrant_machine, loop_opt, solution.getASAPST());
            LOG(fmt::format("Added {} between {} and {}.\n", curOp, prevOp, nextOp));
            iter = solution.getMachineSequence(reentrant_machine).begin() + removalPosition - 1;
            for (auto op : solution.getMachineSequence(reentrant_machine)) {
                std::cout << fmt::to_string(op) << " " << "\n";
            }
            std::cout << fmt::to_string(*iter) << " \n";
        } else {
            iter--;
        }
    }
    output = NoFixedOrderSolution{input.jobOrder, std::move(solution)};
    output.updateDelayGraph(dg);
}

NoFixedOrderSolution IteratedGreedy::solve(problem::Instance &problemInstance,
                                           const cli::CLIArgs &args) {
    // solve the instance
    LOG("Computation of the schedule started");

    a_clock::time_point start = a_clock::now();
    // We only support a single re-entrant machine in the system so choose the first one
    problem::MachineId reentrant_machine = problemInstance.getReEntrantMachines().front();
    if (problemInstance.getMachineOperations(reentrant_machine).size() > 2) {
        throw std::runtime_error("Multiple re-entrancies not implemented yet");
    }

    NoFixedOrderSolution initialSolution;
    IteratedGreedy::createInitialSolution(problemInstance, reentrant_machine, initialSolution);
    // make a copy of the delaygraph
    cg::ConstraintGraph dg = initialSolution.getDelayGraph();
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

        // Search mutations. Randomly choose a search mutator function to apply
        auto chosenMutator = mutator.searchMutators[rand() % mutator.searchMutators.size()];

        // TO DO: validate mutated solution before assigning
        (mutator.*chosenMutator)();
        auto temporarySolution = mutator.output;
        auto ASAPST = algorithms::paths::initializeASAPST(dg);
        PartialSolution solution(
                {{reentrant_machine,
                  temporarySolution.solution.getMachineSequence(reentrant_machine)}},
                {ASAPST});
        auto final_sequence = solution.getAllAndInferredEdges(problemInstance);
        auto result = algorithms::paths::computeASAPST(dg, final_sequence);
        solution.setASAPST(std::move(result.times));

        if (!result.hasPositiveCycle()) {
            currentSolution = temporarySolution; // apply and update solution
        }

        if (currentSolution.solution.getMakespan() <= bestSolution.solution.getMakespan()) {
            std::cout << "updating best solution \n";
            bestSolution = currentSolution;
        }
        end = a_clock::now();
    }
    cg::ConstraintGraph bdg = bestSolution.getDelayGraph();
    for (auto op : bestSolution.solution.getMachineSequence(reentrant_machine)) {
        std::cout << fmt::to_string(op) << "\n";
    }
    return bestSolution;
}

void IteratedGreedy::createInitialSolution(problem::Instance &problemInstance,
                                           problem::MachineId reEntrantMachine,
                                           NoFixedOrderSolution &initialSolution) {

    // randomly initialise job order
    auto jobOrder = problemInstance.getJobsOutput();
    std::shuffle(jobOrder.begin(), jobOrder.end(), std::mt19937(std::random_device()()));
    initialSolution.jobOrder = jobOrder;

    // TO DO: Modify this to check if a given permutation is feasible not the natural fixed order
    // Keep shuffling permutation until we find one that passes
    auto [result, ASAPST] = SolversUtils::checkSolutionAndOutputIfFails(problemInstance);

    Sequence initialSequence;
    problemInstance.updateDelayGraph(cg::Builder::FORPFSSPSD(problemInstance));
    const auto &dg = problemInstance.getDelayGraph();
    problem::ReEntrantId reEntrantMachineId =
            problemInstance.findMachineReEntrantId(reEntrantMachine);

    // check how many operations are mapped
    const std::vector<problem::OperationId> &ops =
            problemInstance.getMachineOperations(reEntrantMachine);

    /// no ordering is required if the number of operations mapped is 1
    if (ops.size() <= 1) {
        throw std::runtime_error(fmt::format("Machine {} is not re-entrant", reEntrantMachine));
    }

    std::optional<problem::JobId> lastDuplexJob;

    // Add all first passes (of _duplex_ jobs) directly followed by their second passes to the
    // initial sequence
    for (auto job : problemInstance.getJobsOutput()) { // For all but the last job
        if (problemInstance.getReEntrancies(job, reEntrantMachineId) == problem::plexity::DUPLEX) {
            const auto &jobOps = problemInstance.getJobOperationsOnMachine(job, reEntrantMachine);

            initialSequence.push_back(jobOps[0]);
            initialSequence.push_back(jobOps[1]);
            lastDuplexJob = job;
        }
    }

    if (!lastDuplexJob.has_value()) {
        throw FmsSchedulerException("Nothing to schedule; only simplex sheets!");
    }

    for (auto op : initialSequence) {
        std::cout << fmt::to_string(op) << "\n";
    }

    // update partial solution
    PartialSolution solution({{reEntrantMachine, initialSequence}}, ASAPST);
    initialSolution.solution = solution;
    initialSolution.jobOrder = jobOrder;
}
