#include "pch/containers.hpp"
#include "pch/utils.hpp"

#include "solvers/sequence.hpp"

#include "FORPFSSPSD/indices.hpp"
#include "FORPFSSPSD/operation.h"
#include "FORPFSSPSD/parsers.hpp"
#include "delayGraph/builder.hpp"
#include "delayGraph/edge.h"
#include "delayGraph/export_utilities.h"
#include "fmsschedulerexception.h"
#include "longest_path.h"
#include "partialsolution.h"

#include <fmt/chrono.h>
#include <fmt/compile.h>
#include <fstream>
#include <nlohmann/json.hpp>

namespace {
void initGraph(FORPFSSPSD::Instance &f, const commandLineArgs &args) {
    if (!f.isGraphInitialized()) {
        f.updateDelayGraph(DelayGraph::Builder::build(f));
    }
    
    auto dg = f.getDelayGraph();
    if (args.verbose >= LOGGER_LEVEL::DEBUG) {
        auto name = fmt::format("input_graph_{}.dot", f.getProblemName());
        DelayGraph::export_utilities::saveAsDot(dg, name);
    }
}

std::tuple<std::vector<PartialSolution>, nlohmann::json>
compute(DelayGraph::delayGraph dg,
        const PartialSolution::MachineEdges &solutionEdges,
        std::string_view problemName) {
    DelayGraph::Edges allEdges;
    for (const auto &[_, edges] : solutionEdges) {
        allEdges.insert(allEdges.end(), edges.begin(), edges.end());
    }

    auto [result, ASAPST] = algorithm::LongestPath::computeASAPST(dg, allEdges);

    if (!result.positiveCycle.empty()) {
        auto result = algorithm::LongestPath::getPositiveCycle(dg, allEdges);
        DelayGraph::export_utilities::saveAsDot(
                dg, fmt::format("infeasible_{}.dot", problemName), allEdges, result);
        LOG_E("The sequence is not valid. It contains a positive cycle.");
        throw FmsSchedulerException("The sequence is not valid");
    }

    if (Logger::getLevel() >= LOGGER_LEVEL::DEBUG) {
        DelayGraph::export_utilities::saveAsDot(
                dg, fmt::format("output_graph_{}.dot", problemName), allEdges);
    }

    PartialSolution solution{solutionEdges, std::move(ASAPST)};
    return std::make_tuple(std::vector<PartialSolution>{{std::move(solution)}}, nlohmann::json{});
}
} // namespace

std::tuple<std::vector<PartialSolution>, nlohmann::json> algorithm::Sequence::solve(
        FORPFSSPSD::Instance &f, const commandLineArgs &args, std::uint64_t iteration) {
    initGraph(f, args);
    const auto solution = loadPerMachineSequencesTop(args.sequenceFile, f, iteration);
    return compute(f.getDelayGraph(), solution, f.getProblemName());
}

std::tuple<std::vector<PartialSolution>, nlohmann::json> algorithm::Sequence::solve(
        FORPFSSPSD::Module &f, const commandLineArgs &args, std::uint64_t iteration) {
    initGraph(f, args);
    const auto solution = loadSingleModuleSequenceTop(args.sequenceFile, f, iteration);
    return compute(f.getDelayGraph(), solution, f.getProblemName());
}

nlohmann::json algorithm::Sequence::loadSequencesTop(const std::string &filename) {
    if (!std::filesystem::exists(filename)) {
        LOG_E("The given sequence file does not exist");
        throw FmsSchedulerException("The given sequence file does not exist");
    }

    auto jsonFile = nlohmann::json::parse(std::ifstream{filename});

    if (!jsonFile.contains(algorithm::Sequence::SequenceStrings::kSequence)) {
        LOG_E("The given sequence file does not contain a {} key",
              algorithm::Sequence::SequenceStrings::kSequence);
        throw FmsSchedulerException("The given sequence file does not contain a sequence key");
    }

    return std::move(jsonFile.at(algorithm::Sequence::SequenceStrings::kSequence));
}

DelayGraph::Edges algorithm::Sequence::loadSingleSequence(const nlohmann::json &jsonSequence,
                                                          const FORPFSSPSD::MachineId machineId,
                                                          const FORPFSSPSD::Instance &f) {
    // TO TEST  : Move the addition of the first operation to source here - just initalise previous op to source and it should fix it
    std::optional<FORPFSSPSD::operation> previousOp;
    DelayGraph::Edges result;
    const auto &dg = f.getDelayGraph();


    for (const auto &[_, operation] : jsonSequence.items()) {
        if (operation.size() != 2) {
            LOG_E("Operation {} is invalid. It must be composed of 2 numbers.", operation.dump());
            throw FmsSchedulerException("The operation is not valid");
        }

        const FS::JobId jobId(operation[0].get<FORPFSSPSD::JobId::ValueType>());
        const FS::OperationId operationId(operation[1].get<FORPFSSPSD::OperationId>());
        const FORPFSSPSD::operation op{jobId, operationId};

        if (!f.containsOp(op)) {
            LOG_E("The operation {} is invalid", op);
            throw FmsSchedulerException("The operation is not valid");
        }

        const auto machineIdProb = f.getMachine(op);
        if (machineIdProb != machineId) {
            LOG_E("The operation {} is not assigned to machine {}", op, machineId);
            throw FmsSchedulerException("The operation is not assigned to the given machine");
        }

        const auto &v = dg.get_vertex(op);
        const auto &vSource = previousOp ? dg.get_vertex(*previousOp) : dg.get_source(machineId);
        const auto w = f.query(vSource, v); // Get weight
        result.emplace_back(vSource.id, v.id, w);
        LOG("Added edge of weight {} from {} to {}", w, vSource.operation, op);
        previousOp = op;
    }

    return result;
}

PartialSolution::MachineEdges algorithm::Sequence::loadPerMachineSequences(
        const nlohmann::json &json, const FORPFSSPSD::Instance &f, const std::uint64_t iteration) {

    if (!json.contains(SequenceStrings::kMachineSequences)) {
        LOG_E("The given object does not contain a {} key", SequenceStrings::kMachineSequences);
        throw FmsSchedulerException(fmt::format("The given object does not contain a {} key",
                                                SequenceStrings::kMachineSequences));
    }

    const auto &sequences = json[SequenceStrings::kMachineSequences];
    PartialSolution::MachineEdges allEdges;
    for (const auto &[machineIdStr, sequence] : sequences.items()) {
        const auto machineId = FORPFSSPSD::parseMachineId(machineIdStr);

        if (sequence.is_array()) {
            allEdges[machineId] = loadSingleSequence(sequence, machineId, f);
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

        allEdges[machineId] = loadSingleSequence(sequence[iterationStr], machineId, f);
    }
    return allEdges;
}

PartialSolution::MachineEdges algorithm::Sequence::loadSingleModuleSequence(
        const nlohmann::json &json, const FORPFSSPSD::Module &f, const std::uint64_t iteration) {
    if (!json.contains(SequenceStrings::kModules)) {
        LOG_E("The given sequence file does not contain any module");
        throw FmsSchedulerException("The given sequence file does not contain any module");
    }

    const auto &modules = json[SequenceStrings::kModules];
    const auto moduleIdStr = fmt::to_string(f.getModuleId());
    return loadPerMachineSequences(modules[moduleIdStr], f, iteration);
}

FMS::ProductionLineSequences
algorithm::Sequence::loadProductionLineSequences(const nlohmann::json &json,
                                                 const FORPFSSPSD::ProductionLine &f) {
    if (!json.contains(SequenceStrings::kModules)) {
        LOG_E("The given sequence file does not contain any module");
        throw FmsSchedulerException("The given sequence file does not contain any module");
    }

    const auto &modules = json[SequenceStrings::kModules];
    FMS::ProductionLineSequences result;
    result.reserve(modules.size());
    
    for (const auto &[moduleIdStr, machineSequences] : modules.items()) {
        const FORPFSSPSD::ModuleId moduleId(std::stoull(moduleIdStr));
        result.emplace(moduleId,
                       loadPerMachineSequences(machineSequences, f.getModule(moduleId), 0));
    }
    return result;
}

nlohmann::json algorithm::Sequence::saveSingleSequence(const DelayGraph::Edges &sequence,
                                                       const DelayGraph::delayGraph &dg) {
    // Assumes that the edges are in order (e.g. (1, 2), (2, 3), (3, 4)...)
    nlohmann::json result = nlohmann::json::array();

    std::optional<FORPFSSPSD::operation> previousOp;

    for (const auto &edge : sequence) {
        if (dg.is_source(edge.src)) {
            const auto op = dg.get_operation(edge.dst);
            previousOp = op;
            result.push_back(nlohmann::json::array({op.jobId.value, op.operationId}));
            continue;
        }

        const auto opSrc = dg.get_operation(edge.src);
        const auto opDst = dg.get_operation(edge.dst);

        if (previousOp != opSrc) {
            LOG_E("The sequence is not valid. It contains non-consecutive nodes.");
            throw FmsSchedulerException("The sequence is not valid");
        }

        result.push_back(nlohmann::json::array({opDst.jobId.value, opDst.operationId}));
        previousOp = opDst;
    }
    return result;
}

nlohmann::json
algorithm::Sequence::savePerMachineSequences(const PartialSolution::MachineEdges &sequences,
                                             const DelayGraph::delayGraph &dg) {
    nlohmann::json result;
    for (const auto &[machineId, sequence] : sequences) {
        result[fmt::to_string(machineId)] = saveSingleSequence(sequence, dg);
    }
    return {{SequenceStrings::kMachineSequences, std::move(result)}};
}

nlohmann::json
algorithm::Sequence::saveProductionLineSequences(const FMS::ModulesSolutions &solutions,
                                                 const FORPFSSPSD::ProductionLine &p) {
    nlohmann::json modules;

    for (const auto &[moduleId, moduleSolution] : solutions) {
        const auto key = fmt::to_string(moduleId);
        const auto &machineEdges = moduleSolution.getChosenEdgesPerMachine();
        modules[key] = savePerMachineSequences(machineEdges, p.getModule(moduleId).getDelayGraph());
    }

    return {{SequenceStrings::kModules, std::move(modules)}};
}

nlohmann::json
algorithm::Sequence::saveProductionLineSequences(const FMS::ProductionLineSequences &sequences,
                                                 const FORPFSSPSD::ProductionLine &f) {
    nlohmann::json modules;

    for (const auto &[moduleId, machineEdges] : sequences) {
        const auto key = fmt::to_string(moduleId);
        modules[key] = savePerMachineSequences(machineEdges, f.getModule(moduleId).getDelayGraph());
    }

    return {{SequenceStrings::kModules, std::move(modules)}};
}