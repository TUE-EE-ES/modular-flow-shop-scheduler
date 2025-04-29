#include "fms/pch/containers.hpp"
#include "fms/pch/fmt.hpp"

#include "fms/solvers/utils.hpp"

#include "fms/algorithms/longest_path.hpp"
#include "fms/cg/builder.hpp"
#include "fms/cg/constraint_graph.hpp"
#include "fms/cg/edge.hpp"
#include "fms/cg/export_utilities.hpp"

using namespace fms;
using namespace fms::solvers;

algorithms::paths::LongestPathResultWithTimes
SolversUtils::checkSolutionAndOutputIfFails(const std::string &fileName,
                                            cg::ConstraintGraph &dg,
                                            const cg::Edges &extraEdges,
                                            const std::string &extraMessage) {
    auto result = algorithms::paths::computeASAPST(dg, extraEdges);
    checkPathResultAndOutputIfFails(fileName, dg, extraMessage, result, extraEdges);
    return result;
}

algorithms::paths::LongestPathResultWithTimes
SolversUtils::checkSolutionAndOutputIfFails(const problem::Instance &instance) {
    const auto &dg = instance.getDelayGraph();
    auto result = algorithms::paths::computeASAPST(dg);

    checkPathResultAndOutputIfFails(
            fmt::format(FMT_COMPILE("input_infeasible_{}"), instance.getProblemName()),
            dg,
            "The input graph is infeasible, aborting.",
            result);

    return result;
}

algorithms::paths::LongestPathResultWithTimes
SolversUtils::checkSolutionAndOutputIfFails(const problem::Instance &instance,
                                            PartialSolution &ps) {
    auto dg = instance.getDelayGraph();
    return checkSolutionAndOutputIfFails(
            fmt::format(FMT_COMPILE("output_infeasible_{}"), instance.getProblemName()),
            dg,
            ps.getAllChosenEdges(instance),
            "The created solution is infeasible!");
}

void SolversUtils::checkPathResultAndOutputIfFails(
        const std::string &fileName,
        const cg::ConstraintGraph &dg,
        const std::string &extraMessage,
        const algorithms::paths::LongestPathResultWithTimes &result,
        const cg::Edges &extraEdges) {
    if (result.hasPositiveCycle()) {
        // Save the file as DOT
        auto name = fileName + ".dot";
        cg::exports::saveAsDot(dg, name, extraEdges, result.positiveCycle);
        throw FmsSchedulerException(extraMessage);
    }
}

PartialSolution SolversUtils::createTrivialSolution(problem::Instance &problem) {
    const auto &reEntrantMachines = problem.getReEntrantMachines();
    MachinesSequences sequences;

    for (const auto &machine : reEntrantMachines) {
        auto machineEdges = createMachineTrivialSolution(problem, machine);
        sequences.emplace(machine, std::move(machineEdges));
    }

    PartialSolution solution(sequences, {});

    cg::ConstraintGraph dg = problem.getDelayGraph();
    auto extraEdges = solution.getAllChosenEdges(problem);
    auto result = algorithms::paths::computeASAPST(dg, extraEdges);
    checkPathResultAndOutputIfFails(
            fmt::format(FMT_COMPILE("output_infeasible_{}"), problem.getProblemName()),
            dg,
            "The created trivial solution is infeasible!",
            result,
            extraEdges);
    solution.setASAPST(std::move(result.times));
    return solution;
}

Sequence SolversUtils::createMachineTrivialSolution(const problem::Instance &problem,
                                                    problem::MachineId machineId) {
    const auto &dg = problem.getDelayGraph();
    const auto &jobs = problem.getJobsOutput();

    Sequence result;
    for (const auto jobId : jobs) {
        const auto &jobMachineOps = problem.getJobOperationsOnMachine(jobId, machineId);
        for (const auto &op : jobMachineOps) {
            result.push_back(op);
        }
    }

    return result;
}

Sequence fms::solvers::SolversUtils::getInferredInputSequence(const problem::Instance &problem,
                                                              const MachinesSequences &sequences) {
    const auto &reEntrantMachines = problem.getReEntrantMachines();
    const auto firstReEntrant = reEntrantMachines.front();
    const auto &firstReEntrantSequence = sequences.at(firstReEntrant);
    return getInferredInputSequence(problem, firstReEntrantSequence);
}

Sequence
fms::solvers::SolversUtils::getInferredInputSequence(const problem::Instance &problem,
                                                     const Sequence &firstReEntrantSequence) {
    const auto &reEntrantMachines = problem.getReEntrantMachines();
    const auto firstReEntrant = reEntrantMachines.front();
    const auto reEntrantId = problem.findMachineReEntrantId(firstReEntrant);

    Sequence inputSequence;
    for (const auto &op : firstReEntrantSequence) {
        const auto plexity = problem.getReEntrancies(op.jobId, reEntrantId);

        const auto &jobMachOps = problem.getJobOperationsOnMachine(op.jobId, firstReEntrant);
        // Ensure that the operation is the first of the job in the machine
        if (jobMachOps.size() <= 0 || jobMachOps.front() != op) {
            continue;
        }

        // Get the first operation of the job
        const auto &firstOp = problem.jobs(op.jobId).front();
        inputSequence.push_back(firstOp);
    }

    return inputSequence;
}

void fms::solvers::SolversUtils::printSequence(const Sequence &sequence) {
    LOG_D(fmt::format("Sequence: [{}]", fmt::join(sequence, ",")));
}

void fms::solvers::SolversUtils::printSequenceIfDebug(const Sequence &sequence) {
    if (!IS_LOG_D()) {
        return;
    }
    printSequence(sequence);
}

cg::Edges fms::solvers::SolversUtils::getEdgesFromSequence(const problem::Instance &problem,
                                                           const Sequence &sequence,
                                                           problem::MachineId machineId) {
    cg::Edges chosenEdges;
    chosenEdges.reserve(sequence.size());

    const auto &g = problem.getDelayGraph();
    std::reference_wrapper<const cg::Vertex> previous = g.getSource(machineId);

    for (std::size_t i = 0; i < sequence.size(); ++i) {
        const auto &op = sequence.at(i);
        const auto &v = g.getVertex(op);

        delay weight{};
        if (op.isMaintenance()) {
            // Adding a maintenance operation extends the duration between the previous
            // and the next operation by the duration of the maintenance operation.
            // Thus, we still need to fetch the time of the normal operation (i.e., without
            // maintenance). This also preserves the sequence-dependent setup times.
            // Ideally, this process should happen inside the "query" function but this function
            // doesn't have a way to know the next operation in the sequence.
            weight = problem.query(previous, g.getVertex(sequence.at(i + 1)));
        } else {
            weight = problem.query(previous, v);
        }
        chosenEdges.emplace_back(previous.get().id, v.id, weight);
        previous = v;
    }

    return chosenEdges;
}

cg::Edges
fms::solvers::SolversUtils::getAllEdgessFromSequences(const problem::Instance &problem,
                                                      const MachinesSequences &sequences) {
    const auto &g = problem.getDelayGraph();
    cg::Edges allEdges;
    for (const auto &[machineId, sequence] : sequences) {
        const auto edges = getEdgesFromSequence(problem, sequence, machineId);
        allEdges.insert(allEdges.end(), edges.begin(), edges.end());
    }

    return allEdges;
}

cg::Edges fms::solvers::SolversUtils::getInferredEdges(const problem::Instance &problem,
                                                       const MachinesSequences &sequences) {
    const auto &sequence = getInferredInputSequence(problem, sequences);
    return getInferredEdgesFromInferredSequence(problem, sequence);
}

cg::Edges
fms::solvers::SolversUtils::getInferredEdges(const problem::Instance &problem,
                                             const Sequence &firstReEntrantMachineSequence) {
    const auto &sequence = getInferredInputSequence(problem, firstReEntrantMachineSequence);
    return getInferredEdgesFromInferredSequence(problem, sequence);
}

cg::Edges
fms::solvers::SolversUtils::getInferredEdgesFromInferredSequence(const problem::Instance &problem,
                                                                 const Sequence &inferredSequence) {
    cg::Edges allEdges;
    allEdges.reserve(inferredSequence.size());

    const auto &g = problem.getDelayGraph();
    std::reference_wrapper<const cg::Vertex> previous = g.getSource(problem.getMachines().front());
    for (const auto &op : inferredSequence) {
        const auto &v = g.getVertex(op);
        const auto weight = problem.query(previous, v);
        allEdges.emplace_back(previous.get().id, v.id, weight);
        previous = v;
    }

    return allEdges;
}

cg::Edges
fms::solvers::SolversUtils::getAllEdgesPlusInferredEdges(const problem::Instance &problem,
                                                         const MachinesSequences &sequences) {
    auto allEdges = getAllEdgessFromSequences(problem, sequences);
    auto inferredEdges = getInferredEdges(problem, sequences);
    allEdges.insert(allEdges.end(), inferredEdges.begin(), inferredEdges.end());
    return allEdges;
}

cg::Edges fms::solvers::SolversUtils::getAllEdgesPlusInferredEdges(
        const problem::Instance &problem, const Sequence &firstReEntrantMachineSequence) {
    auto allEdges = getEdgesFromSequence(
            problem, firstReEntrantMachineSequence, problem.getMachines().front());
    auto inferredEdges = getInferredEdges(problem, firstReEntrantMachineSequence);
    allEdges.insert(allEdges.end(), inferredEdges.begin(), inferredEdges.end());
    return allEdges;
}

algorithms::paths::PathTimes SolversUtils::initProblemGraph(problem::Instance &problemInstance,
                                                            bool saveGraph) {
    if (!problemInstance.isGraphInitialized()) {
        problemInstance.updateDelayGraph(cg::Builder::build(problemInstance));
    }

    if (saveGraph) {
        auto name = fmt::format("input_graph_{}.dot", problemInstance.getProblemName());
        cg::exports::saveAsDot(problemInstance.getDelayGraph(), name);
    }

    auto [result, ASAPST] = SolversUtils::checkSolutionAndOutputIfFails(problemInstance);
    return ASAPST;
}
