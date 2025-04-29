#include "fms/pch/containers.hpp"
#include "fms/pch/fmt.hpp"
#include "fms/pch/utils.hpp"

#include "fms/solvers/sequence.hpp"

#include "fms/algorithms/longest_path.hpp"
#include "fms/cg/builder.hpp"
#include "fms/cg/constraint_graph.hpp"
#include "fms/cg/edge.hpp"
#include "fms/cg/export_utilities.hpp"
#include "fms/problem/indices.hpp"
#include "fms/problem/operation.hpp"
#include "fms/problem/parsers.hpp"
#include "fms/scheduler_exception.hpp"
#include "fms/solvers/partial_solution.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

namespace {
void initGraph(fms::problem::Instance &f) {
    if (!f.isGraphInitialized()) {
        f.updateDelayGraph(fms::cg::Builder::build(f));
    }

    auto dg = f.getDelayGraph();
    if (fms::IS_LOG_D()) {
        auto name = fmt::format("input_graph_{}.dot", f.getProblemName());
        fms::cg::exports::saveAsDot(dg, name);
    }
}

std::tuple<std::vector<fms::solvers::PartialSolution>, nlohmann::json>
compute(const fms::problem::Instance &f,
        const fms::solvers::MachinesSequences &sequences,
        std::string_view problemName) {

    fms::solvers::PartialSolution solution(sequences, {});

    auto dg = f.getDelayGraph();
    const auto allEdges = solution.getAllChosenEdges(f);
    auto result = fms::algorithms::paths::computeASAPST(dg, allEdges);

    if (result.hasPositiveCycle()) {
        auto result = fms::algorithms::paths::getPositiveCycle(dg, allEdges);
        fms::cg::exports::saveAsDot(
                dg, fmt::format("infeasible_{}.dot", problemName), allEdges, result);
        fms::LOG_E("The sequence is not valid. It contains a positive cycle.");
        throw FmsSchedulerException("The sequence is not valid");
    }

    if (fms::IS_LOG_D()) {
        fms::cg::exports::saveAsDot(dg, fmt::format("output_graph_{}.dot", problemName), allEdges);
    }

    solution.setASAPST(std::move(result.times));
    return {std::vector<fms::solvers::PartialSolution>{{std::move(solution)}},
            nlohmann::json::object()};
}
} // namespace

namespace fms::solvers::sequence {

std::tuple<std::vector<PartialSolution>, nlohmann::json>
solve(problem::Instance &f, const cli::CLIArgs &args, std::uint64_t iteration) {
    initGraph(f);
    const auto solution = loadAllMachinesSequencesTop(args.sequenceFile, f, iteration);
    return compute(f, solution, f.getProblemName());
}

std::tuple<std::vector<PartialSolution>, nlohmann::json>
solve(problem::Module &f, const cli::CLIArgs &args, std::uint64_t iteration) {
    initGraph(f);
    const auto solution = loadSingleModuleSequencesTop(args.sequenceFile, f, iteration);
    return compute(f, solution, f.getProblemName());
}

nlohmann::json loadSequencesTop(const std::string &filename) {
    if (!std::filesystem::exists(filename)) {
        LOG_E("The given sequence file does not exist");
        throw FmsSchedulerException("The given sequence file does not exist");
    }

    auto jsonFile = nlohmann::json::parse(std::ifstream{filename});

    if (!jsonFile.contains(SequenceStrings::kSequence)) {
        LOG_E("The given sequence file does not contain a {} key", SequenceStrings::kSequence);
        throw FmsSchedulerException("The given sequence file does not contain a sequence key");
    }

    return std::move(jsonFile.at(SequenceStrings::kSequence));
}

cg::Edges loadMachineEdges(const nlohmann::json &jsonSequence,
                           const problem::MachineId machineId,
                           const problem::Instance &f) {
    // TO TEST  : Move the addition of the first operation to source here - just initalise previous
    // op to source and it should fix it
    std::optional<problem::Operation> previousOp;
    cg::Edges result;
    const auto &dg = f.getDelayGraph();

    for (const auto &[_, operation] : jsonSequence.items()) {
        if (operation.size() != 2) {
            LOG_E("Operation {} is invalid. It must be composed of 2 numbers.", operation.dump());
            throw FmsSchedulerException("The operation is not valid");
        }

        const problem::JobId jobId(operation[0].get<problem::JobId::ValueType>());
        const problem::OperationId operationId(operation[1].get<problem::OperationId>());
        const problem::Operation op{jobId, operationId};

        if (!f.containsOp(op)) {
            LOG_E("The operation {} is invalid", op);
            throw FmsSchedulerException("The operation is not valid");
        }

        const auto machineIdProb = f.getMachine(op);
        if (machineIdProb != machineId) {
            LOG_E("The operation {} is not assigned to machine {}", op, machineId);
            throw FmsSchedulerException("The operation is not assigned to the given machine");
        }

        const auto &v = dg.getVertex(op);
        const auto &vSource = previousOp ? dg.getVertex(*previousOp) : dg.getSource(machineId);
        const auto w = f.query(vSource, v); // Get weight
        result.emplace_back(vSource.id, v.id, w);
        LOG("Added edge of weight {} from {} to {}", w, vSource.operation, op);
        previousOp = op;
    }

    return result;
}

Sequence loadMachineSequence(const nlohmann::json &jsonSequence,
                             const problem::MachineId machineId,
                             const problem::Instance &f) {
    Sequence result;
    for (const auto &[_, operation] : jsonSequence.items()) {
        if (operation.size() != 2) {
            LOG_E("Operation {} is invalid. It must be composed of 2 numbers.", operation.dump());
            throw FmsSchedulerException("The operation is not valid");
        }

        const problem::JobId jobId(operation[0].get<problem::JobId::ValueType>());
        const problem::OperationId operationId(operation[1].get<problem::OperationId>());
        const problem::Operation op{jobId, operationId};

        if (!f.containsOp(op)) {
            LOG_E("The operation {} is invalid", op);
            throw FmsSchedulerException("The operation is not valid");
        }

        const auto machineIdProb = f.getMachine(op);
        if (machineIdProb != machineId) {
            LOG_E("The operation {} is not assigned to machine {}", op, machineId);
            throw FmsSchedulerException("The operation is not assigned to the given machine");
        }

        result.push_back(op);
    }

    return result;
}

MachineEdges loadAllMachinesEdges(const nlohmann::json &json,
                                  const problem::Instance &f,
                                  const std::uint64_t iteration) {

    if (!json.contains(SequenceStrings::kMachineSequences)) {
        LOG_E("The given object does not contain a {} key", SequenceStrings::kMachineSequences);
        throw FmsSchedulerException(fmt::format("The given object does not contain a {} key",
                                                SequenceStrings::kMachineSequences));
    }

    const auto &sequences = json[SequenceStrings::kMachineSequences];
    MachineEdges allEdges;
    for (const auto &[machineIdStr, sequence] : sequences.items()) {
        const auto machineId = problem::parseMachineId(machineIdStr);

        if (sequence.is_array()) {
            allEdges[machineId] = loadMachineEdges(sequence, machineId, f);
            continue;
        }

        // It is an object, thus the key is the iteration number and the value is the sequence.
        // If the iteration goes above the given number of sequences, we wrap around.
        const auto currentIteration = iteration % sequence.size();
        const auto iterationStr = fmt::to_string(currentIteration);

        if (!sequence.contains(iterationStr)) {
            LOG_E("The given sequence file does not contain the iteration {}", currentIteration);
            throw FmsSchedulerException("The given sequence file does not contain the iteration");
        }

        allEdges[machineId] = loadMachineEdges(sequence[iterationStr], machineId, f);
    }
    return allEdges;
}

MachinesSequences loadAllMachinesSequences(const nlohmann::json &json,
                                           const problem::Instance &f,
                                           const std::uint64_t iteration) {
    if (!json.contains(SequenceStrings::kMachineSequences)) {
        LOG_E("The given object does not contain a {} key", SequenceStrings::kMachineSequences);
        throw FmsSchedulerException(fmt::format("The given object does not contain a {} key",
                                                SequenceStrings::kMachineSequences));
    }

    const auto &sequences = json[SequenceStrings::kMachineSequences];

    MachinesSequences allSequences;
    for (const auto &[machineIdStr, sequence] : sequences.items()) {
        const auto machineId = problem::parseMachineId(machineIdStr);

        if (sequence.is_array()) {
            allSequences[machineId] = loadMachineSequence(sequence, machineId, f);
            continue;
        }

        // It is an object, thus the key is the iteration number and the value is the sequence.
        // If the iteration goes above the given number of sequences, we wrap around.
        const auto currentIteration = iteration % sequence.size();
        const auto iterationStr = fmt::to_string(currentIteration);

        if (!sequence.contains(iterationStr)) {
            LOG_E("The given sequence file does not contain the iteration {}", currentIteration);
            throw FmsSchedulerException("The given sequence file does not contain the iteration");
        }

        allSequences[machineId] = loadMachineSequence(sequence[iterationStr], machineId, f);
    }

    return allSequences;
}

MachineEdges loadSingleModuleEdges(const nlohmann::json &json,
                                   const problem::Module &f,
                                   const std::uint64_t iteration) {
    if (!json.contains(SequenceStrings::kModules)) {
        LOG_E("The given sequence file does not contain any module");
        throw FmsSchedulerException("The given sequence file does not contain any module");
    }

    const auto &modules = json[SequenceStrings::kModules];
    const auto moduleIdStr = fmt::to_string(f.getModuleId());
    return loadAllMachinesEdges(modules[moduleIdStr], f, iteration);
}

MachinesSequences loadSingleModuleSequences(const nlohmann::json &json,
                                            const problem::Module &f,
                                            const std::uint64_t iteration) {
    if (!json.contains(SequenceStrings::kModules)) {
        LOG_E("The given sequence file does not contain any module");
        throw FmsSchedulerException("The given sequence file does not contain any module");
    }

    const auto &modules = json[SequenceStrings::kModules];
    const auto moduleIdStr = fmt::to_string(f.getModuleId());
    return loadAllMachinesSequences(modules[moduleIdStr], f, iteration);
}

ProductionLineEdges loadProductionLineEdges(const nlohmann::json &json,
                                            const problem::ProductionLine &f) {
    if (!json.contains(SequenceStrings::kModules)) {
        LOG_E("The given sequence file does not contain any module");
        throw FmsSchedulerException("The given sequence file does not contain any module");
    }

    const auto &modules = json[SequenceStrings::kModules];
    ProductionLineEdges result;
    result.reserve(modules.size());

    for (const auto &[moduleIdStr, machineSequences] : modules.items()) {
        const problem::ModuleId moduleId(std::stoull(moduleIdStr));
        result.emplace(moduleId, loadAllMachinesEdges(machineSequences, f.getModule(moduleId), 0));
    }
    return result;
}

nlohmann::json saveMachineSequence(const cg::Edges &sequence, const cg::ConstraintGraph &dg) {
    // Assumes that the edges are in order (e.g. (1, 2), (2, 3), (3, 4)...)
    nlohmann::json result = nlohmann::json::array();

    std::optional<problem::Operation> previousOp;

    for (const auto &edge : sequence) {
        if (dg.isSource(edge.src)) {
            const auto op = dg.getOperation(edge.dst);
            if (!op.isValid()) {
                LOG_E("The operation {} is invalid", op);
                throw FmsSchedulerException("The operation is not valid");
            }

            previousOp = op;
            result.push_back(nlohmann::json::array({op.jobId.value, op.operationId}));
            continue;
        }

        const auto opSrc = dg.getOperation(edge.src);
        const auto opDst = dg.getOperation(edge.dst);

        if (previousOp != opSrc) {
            LOG_E("The sequence is not valid. It contains non-consecutive nodes.");
            throw FmsSchedulerException("The sequence is not valid");
        }

        result.push_back(nlohmann::json::array({opDst.jobId.value, opDst.operationId}));
        previousOp = opDst;
    }
    return result;
}

nlohmann::json saveMachineSequence(const Sequence &sequence) {
    nlohmann::json result = nlohmann::json::array();
    for (const auto &op : sequence) {
        result.push_back(nlohmann::json::array({op.jobId.value, op.operationId}));
    }
    return result;
}

nlohmann::json saveMachineSequence(const std::vector<cg::Edges> &sequences,
                                   const cg::ConstraintGraph &dg) {
    nlohmann::json result = nlohmann::json::array();
    for (std::size_t i = 0; i < sequences.size(); ++i) {
        result[i] = saveMachineSequence(sequences[i], dg);
    }
    return result;
}

nlohmann::json saveMachineSequence(const std::vector<Sequence> &sequences) {
    nlohmann::json result = nlohmann::json::array();
    for (std::size_t i = 0; i < sequences.size(); ++i) {
        result[i] = saveMachineSequence(sequences[i]);
    }
    return result;
}

nlohmann::json saveAllMachinesSequences(const MachineEdges &sequences,
                                        const cg::ConstraintGraph &dg) {
    nlohmann::json result;
    for (const auto &[machineId, sequence] : sequences) {
        result[fmt::to_string(machineId)] = saveMachineSequence(sequence, dg);
    }
    return {{SequenceStrings::kMachineSequences, std::move(result)}};
}

nlohmann::json saveAllMachinesSequences(const MachinesSequences &sequences) {
    nlohmann::json result;
    for (const auto &[machineId, sequence] : sequences) {
        result[fmt::to_string(machineId)] = saveMachineSequence(sequence);
    }
    return {{SequenceStrings::kMachineSequences, std::move(result)}};
}

nlohmann::json saveAllMachinesSequences(const std::vector<MachineEdges> &sequences,
                                        const cg::ConstraintGraph &g) {
    nlohmann::json result;

    // We must change it from a vector of MachineEdges to a vector of Edges for each machine

    std::unordered_map<problem::MachineId, std::vector<cg::Edges>> allEdges;
    for (const auto &machineEdges : sequences) {
        for (const auto &[machineId, edges] : machineEdges) {
            allEdges[machineId].push_back(edges);
        }
    }

    for (const auto &[machineId, edges] : allEdges) {
        result[fmt::to_string(machineId)] = saveMachineSequence(edges, g);
    }

    return {{SequenceStrings::kMachineSequences, std::move(result)}};
}

nlohmann::json saveAllMachinesSequences(const std::vector<MachinesSequences> &sequences) {
    nlohmann::json result;

    // We must change it from a vector of MachinesSequences to a vector of Sequences for each
    // machine

    std::unordered_map<problem::MachineId, std::vector<Sequence>> allSequences;
    for (const auto &machineSequences : sequences) {
        for (const auto &[machineId, sequence] : machineSequences) {
            allSequences[machineId].push_back(sequence);
        }
    }

    for (const auto &[machineId, sequences] : allSequences) {
        result[fmt::to_string(machineId)] = saveMachineSequence(sequences);
    }

    return {{SequenceStrings::kMachineSequences, std::move(result)}};
}

nlohmann::json saveProductionLineSequences(const ModulesSolutions &solutions,
                                           const problem::ProductionLine &p) {
    nlohmann::json modules;

    for (const auto &[moduleId, moduleSolution] : solutions) {
        const auto key = fmt::to_string(moduleId);
        const auto &machinesSequences = moduleSolution.getChosenSequencesPerMachine();
        const auto &g = p.getModule(moduleId).getDelayGraph();
        modules[key] = saveAllMachinesSequences(machinesSequences);
    }

    return {{SequenceStrings::kModules, std::move(modules)}};
}

nlohmann::json saveProductionLineSequences(const std::vector<ModulesSolutions> &solutions,
                                           const problem::ProductionLine &p) {
    nlohmann::json modules;

    // We must change it from a vector of ModulesSolutions to a vector of MachineEdges for each
    // module

    std::unordered_map<problem::ModuleId, std::vector<MachinesSequences>> allSequences;
    for (const auto &moduleSolutions : solutions) {
        for (const auto &[moduleId, moduleSolution] : moduleSolutions) {
            allSequences[moduleId].push_back(moduleSolution.getChosenSequencesPerMachine());
        }
    }

    for (const auto &[moduleId, machinesSequences] : allSequences) {
        const auto key = fmt::to_string(moduleId);
        const auto &g = p.getModule(moduleId).getDelayGraph();
        modules[key] = saveAllMachinesSequences(machinesSequences);
    }

    return {{SequenceStrings::kModules, std::move(modules)}};
}

nlohmann::json saveProductionLineSequences(const ProductionLineSolution &solution,
                                           const problem::ProductionLine &f) {
    return saveProductionLineSequences(solution.getSolutions(), f);
}

nlohmann::json saveProductionLineSequences(const ProductionLineEdges &sequences,
                                           const problem::ProductionLine &f) {
    nlohmann::json modules;

    for (const auto &[moduleId, machineEdges] : sequences) {
        const auto key = fmt::to_string(moduleId);
        modules[key] =
                saveAllMachinesSequences(machineEdges, f.getModule(moduleId).getDelayGraph());
    }

    return {{SequenceStrings::kModules, std::move(modules)}};
}

} // namespace fms::solvers::sequence
