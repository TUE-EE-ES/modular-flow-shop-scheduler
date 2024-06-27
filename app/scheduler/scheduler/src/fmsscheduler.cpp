#include "pch/containers.hpp"
#include "pch/utils.hpp"

#include "fmsscheduler.h"

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "FORPFSSPSD/module.hpp"
#include "FORPFSSPSD/xmlParser.h"
#include "delayGraph/delayGraph.h"
#include "partialsolution.h"
#include "solvers/anytimeheuristic.h"
#include "solvers/branchbound.h"
#include "solvers/broadcast_line_solver.hpp"
#include "solvers/cocktail_line_solver.hpp"
#include "solvers/dd.hpp"
#include "solvers/forwardheuristic.h"
#include "solvers/iteratedgreedy.h"
#include "solvers/mneh-heuristic.h"
#include "solvers/paretoheuristic.h"
#include "solvers/sequence.hpp"
#include "solvers/simple.hpp"
#include "versioning.h"

#include <chrono>
#include <fmt/compile.h>
#include <iostream>

using namespace rapidxml;
using namespace nlohmann;
using namespace DelayGraph;

void FmsScheduler::compute(commandLineArgs &args) {
    FORPFSSPSDXmlParser parser(args.inputFile);
    switch (parser.getFileType()) {
    case FORPFSSPSDXmlParser::FileType::MODULAR:
        computeModular(args, std::move(parser));
        break;
    case FORPFSSPSDXmlParser::FileType::SHOP: {
        computeShop(args, std::move(parser));
        break;
    }
    }
}

FORPFSSPSD::Instance FmsScheduler::loadFlowShopInstance(commandLineArgs &args,
                                                        FORPFSSPSDXmlParser &parser) {
    FORPFSSPSD::Instance instance = parser.createFlowShop(args.shopType);

    if (!args.maintPolicyFile.empty()) {
        FORPFSSPSDXmlParser::loadMaintenancePolicy(instance, args.maintPolicyFile);
    }
    return instance;
}

std::pair<bool, std::vector<delay>>
FmsScheduler::checkConsistency(const FORPFSSPSD::Instance &flowshop) {
    bool bounds = true; // consistent, unless found otherwise
    const delayGraph &dg = flowshop.getDelayGraph();

    for (const auto &[jobId, ops] : flowshop.jobs()) {
        const auto &prevOp = ops.front();

        for (const auto &op : ops | std::views::drop(1)) {
            // check the intra-job constraints
            if (!dg.has_edge(op, prevOp)) {
                continue;
            }

            // check intra-job constraints
            edge minimumSetupTime = dg.get_edge(prevOp, op);
            edge deadline = dg.get_edge(op, prevOp);
            if (minimumSetupTime.weight + deadline.weight > 0) {
                bounds = false; // deadline cannot be satisfied locally.
                LOG(LOGGER_LEVEL::WARNING,
                    fmt::format(FMT_COMPILE("Deadline between {} and {} cannot be "
                                            "satisfied ({} > {})\n"),
                                prevOp,
                                op,
                                minimumSetupTime.weight,
                                -deadline.weight));
            }
        }
    }

    auto vec = algorithm::LongestPath::initializeASAPST(dg);
    auto result = algorithm::LongestPath::computeASAPST(dg, vec);

    bounds = bounds && result.positiveCycle.empty();
    // earliest possible start times, given no interleavings;
    return {bounds, std::move(vec)};
}

std::tuple<std::vector<PartialSolution>, nlohmann::json>
FmsScheduler::runAlgorithm(FORPFSSPSD::Instance &flowShopInstance,
                           const commandLineArgs &args,
                           const std::uint64_t iteration) {
    switch (args.algorithm) {
    case AlgorithmType::BHCS:
    case AlgorithmType::MIBHCS:
    case AlgorithmType::MISIM:
    case AlgorithmType::ASAP:
    case AlgorithmType::MIASAP:
    case AlgorithmType::MIASAPSIM:
        return {{algorithm::ForwardHeuristic::solve(flowShopInstance, args)}, {}};
    case AlgorithmType::MDBHCS:
        return {algorithm::ParetoHeuristic::solve(flowShopInstance, args), {}};
    case AlgorithmType::BRANCH_BOUND:
        return {{algorithm::BranchBound::solve(flowShopInstance, args)}, {}};
    case AlgorithmType::ANYTIME:
        return {{algorithm::AnytimeHeuristic::solve(flowShopInstance, args)}, {}};
    case AlgorithmType::ITERATED_GREEDY:
        return {{algorithm::IteratedGreedy::solve(flowShopInstance, args).solution}, {}};
    case AlgorithmType::NEH:
    case AlgorithmType::MINEH:
    case AlgorithmType::MINEHSIM:
        return {{algorithm::MNEH::solve(flowShopInstance, args)}, {}};
    case AlgorithmType::DD:
        return algorithm::DDSolver::solve(flowShopInstance, args);
    case AlgorithmType::GIVEN_SEQUENCE:
        return algorithm::Sequence::solve(flowShopInstance, args, iteration);
    case AlgorithmType::SIMPLE:
        return algorithm::SimpleScheduler::solve(flowShopInstance, args);
    default:
        throw std::runtime_error(
                fmt::format("FmsScheduler::runAlgorithm: algorithm '{}' not supported",
                            args.algorithm.shortName()));
    }
}

std::tuple<std::vector<PartialSolution>, nlohmann::json>
FmsScheduler::runAlgorithm(FORPFSSPSD::Module &flowShopInstance,
                           const commandLineArgs &args,
                           const std::uint64_t iteration) {
    if (args.algorithm == AlgorithmType::GIVEN_SEQUENCE) {
        // Given sequence behaves differently if it is a module or a normal instance
        return algorithm::Sequence::solve(flowShopInstance, args, iteration);
    }
    return runAlgorithm(static_cast<FORPFSSPSD::Instance &>(flowShopInstance), args);
}

std::tuple<std::vector<FMS::ProductionLineSolution>, nlohmann::json>
FmsScheduler::runAlgorithm(FORPFSSPSD::ProductionLine &problemInstance,
                           const commandLineArgs &args) {
    // The modular scheduler may use any algorithm so we cannot call the modular solver
    // within the runAlgorithm method
    switch (args.modularAlgorithm) {
    case ModularAlgorithmType::BROADCAST:
        return algorithm::BroadcastLineSolver::solve(problemInstance, args);
    case ModularAlgorithmType::COCKTAIL:
    default:
        return algorithm::CocktailLineSolver::solve(problemInstance, args);
    }
}

void FmsScheduler::saveSolution(const PartialSolution &solution,
                                const commandLineArgs &args,
                                const FORPFSSPSD::Instance &problem,
                                nlohmann::json &data) {
    LOG("Saving the timing schedule(s) for the scheduling problem");

    if (args.outputFormat == ScheduleOutputFormat::JSON
        || args.outputFormat == ScheduleOutputFormat::CBOR) {
        nlohmann::json schedule;

        for (const auto &[jobId, ops] : problem.jobs()) {
            for (const auto &op : ops) {
                const auto vId = problem.getDelayGraph().get_vertex_id(op);
                const auto startTime = solution.getASAPST().at(vId);
                schedule[fmt::to_string(jobId)][fmt::to_string(op.operationId)] = startTime;
            }
        }
        data["schedule"] = schedule;
        data.update(algorithm::Sequence::savePerMachineSequencesTop(
                solution.getChosenEdgesPerMachine(), problem.getDelayGraph()));

    } else {
        auto tmpArgs = args;
        tmpArgs.outputFile = fmt::format("{}.best", args.outputFile);
        problem.save(solution, tmpArgs);
    }
}

void FmsScheduler::saveSolution(const FMS::ProductionLineSolution &solution,
                                const commandLineArgs & /*args*/,
                                const FORPFSSPSD::ProductionLine &problem,
                                nlohmann::json &data) {
    nlohmann::json schedule;
    for (const auto &[moduleId, module] : problem.modules()) {
        const auto &moduleSolution = solution[moduleId];
        for (const auto &[jobId, ops] : module.jobs()) {
            for (const auto &op : ops) {
                const auto vId = module.getDelayGraph().get_vertex_id(op);
                const auto startTime = moduleSolution.getASAPST().at(vId);
                schedule[fmt::to_string(moduleId.value)][fmt::to_string(jobId)]
                        [fmt::to_string(op.operationId)] = startTime;
            }
        }
    }
    data["solution"] = schedule;
    data.update(algorithm::Sequence::saveProductionLineSequencesTop(solution, problem));
}

void FmsScheduler::computeShop(commandLineArgs &args, FORPFSSPSDXmlParser parser) {
    FORPFSSPSD::Instance flowshopInstance = loadFlowShopInstance(args, parser);

    LOG(">> {} SELECTED <<", args.algorithm.fullName());
    LOG("Solving the scheduling problem instance\n");

    fmt::print(FMT_COMPILE("Solving {}\n"), flowshopInstance.getProblemName());
    solveAndSave(flowshopInstance, args);
}

void FmsScheduler::computeModular(commandLineArgs &args, FORPFSSPSDXmlParser parser) {
    FORPFSSPSD::ProductionLine productionLine = parser.createProductionLine(args.shopType);
    LOG(">> {} SELECTED <<", args.modularAlgorithm.fullName());
    solveAndSave(productionLine, args);
}

nlohmann::json FmsScheduler::initializeData(const commandLineArgs &args) {
    return {{"solved", false},
            {"timeout", false},
            {"productivity", args.productivityWeight},
            {"flexibility", args.flexibilityWeight},
            {"timeOutValue", args.timeOut.count()},
            {"version", VERSION}};
}

void FmsScheduler::saveJSONFile(const nlohmann::json &data, const commandLineArgs &args) {
    std::ofstream jsonFile(args.outputFile + ".json", std::ios::out);
    jsonFile << data.dump(4);
    jsonFile.close();
}

void FmsScheduler::saveCBORFile(const nlohmann::json &data, const commandLineArgs &args) {
    std::ofstream cborFile(args.outputFile + ".cbor", std::ios::binary | std::ios::out);
    json::to_cbor(data, cborFile);
    cborFile.close();
}
