#include "fms/pch/containers.hpp"
#include "fms/pch/fmt.hpp"

#include "fms/solvers/mneh_heuristic.hpp"

#include "fms/algorithms/longest_path.hpp"
#include "fms/cg/builder.hpp"
#include "fms/cg/constraint_graph.hpp"
#include "fms/cg/edge.hpp"
#include "fms/cg/export_utilities.hpp"
#include "fms/problem/flow_shop.hpp"
#include "fms/problem/indices.hpp"
#include "fms/problem/operation.hpp"
#include "fms/solvers/asap_backtrack.hpp"
#include "fms/solvers/asap_cs.hpp"
#include "fms/solvers/forward_heuristic.hpp"
#include "fms/solvers/maintenance_heuristic.hpp"
#include "fms/solvers/utils.hpp"

using namespace fms;
using namespace fms::solvers;

namespace {
Sequence createInitialSequenceUsingBhcs(problem::Instance &problem, const cli::CLIArgs &args) {
    // Copy arguments
    auto argsC = args;
    argsC.algorithm = cli::AlgorithmType::BHCS;

    if (args.algorithm == cli::AlgorithmType::MNEH_BHCS_FLEXIBLE) {
        // Focus only on flexibility. Productivity and tie-breaking are not important because we
        // only want to seed MNEH. This is effectively a completely different algorithm than the
        // BHCS algorithm published in the papers. It just finds a sequence.

        // The flexibility parameter basically tries to minimize the increase in start time of the
        // current operation when this operation is inserted at the current position in the
        // scheduling sequence. In contrast, the productivity parameter tries to minimize the effect
        // of increasing the start time of the next operation when the current operation is inserted
        // at the current position in the scheduling sequence. Thus increasing the flexibility
        // weight gives a more flexible schedule, while increasing the productivity weight gives a
        // more productive schedule (aka minimizing the makespan).
        argsC.flexibilityWeight = 1;
        argsC.productivityWeight = 0;
        argsC.tieWeight = 0;
    } else if (args.algorithm == cli::AlgorithmType::MNEH_BHCS_COMBI) {
        argsC.flexibilityWeight = 2; // Increase flexibility to allow more options for MNEH
    }

    auto result = forward::solve(problem, argsC);
    return result.getMachineSequence(problem.getReEntrantMachines().front());
}

Sequence createInitialSequenceUsingAsap(problem::Instance &problem, const cli::CLIArgs &args) {
    // Copy arguments
    auto argsC = args;
    argsC.algorithm = cli::AlgorithmType::ASAP;
    auto result = ASAPCS::solve(problem, argsC);
    return result.getMachineSequence(problem.getReEntrantMachines().front());
}

Sequence createInitialSequenceUsingAsapBacktrack(problem::Instance &problem,
                                                 const cli::CLIArgs &args) {
    // Copy arguments
    auto argsC = args;
    argsC.algorithm = cli::AlgorithmType::ASAP_BACKTRACK;
    // Limit the time for the ASAP backtrack to 1 second per job
    argsC.timeOut = std::chrono::milliseconds(1000);
    auto result = AsapBacktrack::solve(problem, argsC);
    return result.getMachineSequence(problem.getReEntrantMachines().front());
}

Sequence obtainInitialSequenceInternal(problem::Instance &problem,
                                       problem::MachineId reEntrantMachine,
                                       const cli::CLIArgs &args) {
    if (args.algorithm == cli::AlgorithmType::MNEH_BHCS_COMBI
        || args.algorithm == cli::AlgorithmType::MNEH_BHCS_FLEXIBLE) {
        return createInitialSequenceUsingBhcs(problem, args);
    } else if (args.algorithm == cli::AlgorithmType::MNEH_ASAP) {
        return createInitialSequenceUsingAsap(problem, args);
    } else if (args.algorithm == cli::AlgorithmType::MNEH_ASAP_BACKTRACK) {
        return createInitialSequenceUsingAsapBacktrack(problem, args);
    } else {
        auto trivialSolution = SolversUtils::createTrivialSolution(problem);
        return trivialSolution.getMachineSequence(reEntrantMachine);
    }
}

Sequence obtainInitialSequence(problem::Instance &problem,
                               problem::MachineId reEntrantMachine,
                               const cli::CLIArgs &args) {
    auto sequence = obtainInitialSequenceInternal(problem, reEntrantMachine, args);
    SolversUtils::printSequenceIfDebug(sequence);
    return sequence;
}

} // namespace

PartialSolution MNEH::solve(problem::Instance &problem, const cli::CLIArgs &args) {
    // solve the instance
    LOG("Computation of the schedule started");
    SolversUtils::initProblemGraph(problem, IS_LOG_D());

    // We only support a single re-entrant machine in the system so choose the first one
    problem::MachineId reEntrantMachine = problem.getReEntrantMachines().front();
    if (problem.getMachineOperations(reEntrantMachine).size() > 2) {
        throw std::runtime_error("Multiple re-entrancies not implemented yet");
    }

    auto dg = problem.getDelayGraph();
    auto seedSequence = obtainInitialSequence(problem, reEntrantMachine, args);
    auto chosenSequence = MNEH::improveSequence(problem, reEntrantMachine, seedSequence, dg, args);

    PartialSolution solution({{reEntrantMachine, std::move(chosenSequence)}}, {});
    auto edges = solution.getAllAndInferredEdges(problem);
    auto chosenResult = algorithms::paths::computeASAPST(dg, edges);

    if (chosenResult.hasPositiveCycle()) {
        throw FmsSchedulerException("Chosen sequence is infeasible");
    }
    solution.setASAPST(std::move(chosenResult.times));

    switch (args.algorithm) {
    case cli::AlgorithmType::MINEH:
    case cli::AlgorithmType::MINEHSIM:
        LOG("final maint check");
        for (auto edge : solution.getAllChosenEdges(problem)) {
            LOG("before: {}->{}", dg.getVertex(edge.src), dg.getVertex(edge.dst));
        }
        std::tie(solution, dg) =
                maintenance::triggerMaintenance(dg, problem, reEntrantMachine, solution, args);
        problem.updateDelayGraph(dg);
        for (auto edge : solution.getAllChosenEdges(problem)) {
            LOG("after: {}->{}", dg.getVertex(edge.src), dg.getVertex(edge.dst));
        }
        break;
    default:
        break;
    }

    if (IS_LOG_D()) {
        auto name = fmt::format("output_graph_mneh_{}.dot", problem.getProblemName());
        cg::exports::saveAsDot(problem, solution, name);
    }

    return solution;
}

Sequence MNEH::improveSequence(problem::Instance &problem,
                               problem::MachineId reEntrantMachine,
                               const Sequence &seedSequence,
                               cg::ConstraintGraph &dg,
                               const cli::CLIArgs &args) {
    // seed sequence must be valid
    PartialSolution seedSolution({{reEntrantMachine, seedSequence}}, {});
    auto edges = seedSolution.getAllAndInferredEdges(problem);
    auto result = algorithms::paths::computeASAPST(dg, std::move(edges));
    seedSolution.setASAPST(std::move(result.times));

    if (result.hasPositiveCycle()) {
        throw FmsSchedulerException("seed sequence infeasible \n");
    }

    auto [builtSequence, builtSolution] =
            updateSequence(problem, reEntrantMachine, seedSequence, dg, args);

    switch (args.algorithm) {
    case cli::AlgorithmType::MINEH: {
        auto [builtMaintSolution, builtMaintDg] =
                maintenance::triggerMaintenance(dg, problem, reEntrantMachine, builtSolution, args);
        builtSolution = builtMaintSolution;
        auto [seedMaintSolution, seedMaintDg] =
                maintenance::triggerMaintenance(dg, problem, reEntrantMachine, seedSolution, args);
        seedSolution = seedMaintSolution;
        break;
    }
    default: // do nothing
        break;
    }

    delay currMakespan = seedSolution.getRealMakespan(problem);
    Sequence bestSequence = builtSequence;
    std::size_t iteration = 0;

    while (builtSolution.getRealMakespan(problem) < currMakespan && iteration < args.maxIterations) {
        // update before change
        currMakespan = builtSolution.getRealMakespan(problem);

        bestSequence = builtSequence;
        std::tie(builtSequence, builtSolution) =
                updateSequence(problem, reEntrantMachine, builtSequence, dg, args);

        switch (args.algorithm) {
        case cli::AlgorithmType::MINEH: {
            auto [builtMaintSolution, builtMaintDg] = maintenance::triggerMaintenance(
                    dg, problem, reEntrantMachine, builtSolution, args);
            builtSolution = builtMaintSolution;
            break;
        }
        default: // do nothing
            break;
        }

		iteration++;
    }
    return bestSequence;
}

std::tuple<Sequence, PartialSolution> MNEH::updateSequence(const problem::Instance &problem,
                                                           problem::MachineId reEntrantMachine,
                                                           const Sequence &seedSequence,
                                                           cg::ConstraintGraph &dg,
                                                           const cli::CLIArgs &args) {

    // first option just goes into the built sequence directly
    Sequence builtSequence = {seedSequence[0]};

    if (IS_LOG_D()) {
        LOG_D("Updating sequence from seed sequence");
        for (auto e : seedSequence) {
            LOG_D(FMT_COMPILE("{} "), e);
        }
    }

    // initialise seed sequence performance
    auto ASAPST = algorithms::paths::initializeASAPST(dg);
    PartialSolution seedSolution({{reEntrantMachine, seedSequence}}, ASAPST);
    auto finalSeedSequence = seedSolution.getAllAndInferredEdges(problem);
    auto [resultseed, ASAPSTseed] = algorithms::paths::computeASAPST(dg, finalSeedSequence);
    seedSolution.setASAPST(ASAPSTseed);
    delay minMakespan = seedSolution.getRealMakespan(problem);

    for (int j = 1; j < seedSequence.size(); j++) {
        auto currO = seedSequence[j];

        std::optional<Sequence> bestSequence;

        for (std::ptrdiff_t i = 0; i <= builtSequence.size(); i++) {
            Sequence testSequence = builtSequence;

            if (i < builtSequence.size()) {
                // insert in position i of built sequence
                testSequence.insert(testSequence.begin() + i, currO);
                LOG_D(FMT_COMPILE("Inserting operation {} after {}"), currO, builtSequence[i]);
            } else {
                // insert at end of built sequence
                testSequence.push_back(currO);
                LOG_D(FMT_COMPILE("Inserting operation {} after {}"), currO, builtSequence.back());
            }

            // add the rest of the initial sequence
            // mend connection
            Sequence evaluateSequence = testSequence;
            evaluateSequence.insert(
                    evaluateSequence.end(), seedSequence.begin() + j + 1, seedSequence.end());

            // compute if valid and better than others only accept the responsible test sequence if
            // evaluate has minimum makespan seen so far
            if (validateSequence(problem, evaluateSequence, reEntrantMachine, dg)) {
                PartialSolution solution({{reEntrantMachine, std::move(evaluateSequence)}}, {});
                auto finalSequence = solution.getAllAndInferredEdges(problem);
                auto result = algorithms::paths::computeASAPST(dg, finalSequence);
                solution.setASAPST(std::move(result.times));

                if (!result.hasPositiveCycle()) {
                    const delay newMakespan = solution.getRealMakespan(problem);
                    if (newMakespan < minMakespan) {
                        bestSequence = std::move(testSequence);
                        minMakespan = newMakespan;
                    }
                }
            }
        }

        if (bestSequence.has_value()) {
            builtSequence = bestSequence.value();
        } else {
            builtSequence.push_back(currO);
        }

        if (IS_LOG_D()) {
            LOG_D("Chosen sub sequence");
            for (auto e : builtSequence) {
                LOG_D(FMT_COMPILE("{} "), e);
            }
        }
    }

    PartialSolution builtSolution({{reEntrantMachine, builtSequence}}, {});
    auto finalSequence = builtSolution.getAllAndInferredEdges(problem);
    auto [result, ASAPSTr] = algorithms::paths::computeASAPST(dg, finalSequence);
    builtSolution.setASAPST(ASAPSTr);
    return {builtSequence, builtSolution};
}

bool MNEH::validateSequence(const problem::Instance &problemInstance,
                            const Sequence &sequence,
                            problem::MachineId reEntrantMachine,
                            const cg::ConstraintGraph &dg) {

    bool status = true;
    // check how many operations are mapped
    const auto &ops = problemInstance.getMachineOperations(reEntrantMachine);

    std::optional<problem::JobId> lastFirstPass;
    std::optional<problem::JobId> lastSecondPass;
    std::unordered_set<problem::JobId> doneFirstPass;
    for (auto currOp : sequence) {
        const auto &curr_v = dg.getVertex(currOp);

        // confirm order of first passes
        if (currOp.operationId == ops[0]) { // isFirstPass
            if (lastFirstPass.has_value() && currOp.jobId <= lastFirstPass.value()) {
                return false;
            }
            lastFirstPass = currOp.jobId;
            doneFirstPass.insert(currOp.jobId);
        }

        // confirm order of second passes
        // confirm first second pass precedence
        if (currOp.operationId == ops[1]) { // isSecondPass
            if (!doneFirstPass.contains(currOp.jobId)) {
                return false;
            }
            if (lastSecondPass.has_value() && currOp.jobId <= lastSecondPass.value()) {
                return false;
            }
            lastSecondPass = currOp.jobId;
        }
    }
    return status;
}
