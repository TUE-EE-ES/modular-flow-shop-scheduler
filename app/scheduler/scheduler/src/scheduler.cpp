#include "fms/pch/containers.hpp"
#include "fms/pch/fmt.hpp"
#include "fms/pch/utils.hpp"

#include "fms/scheduler.hpp"

#include "fms/cg/constraint_graph.hpp"
#include "fms/cli/command_line.hpp"
#include "fms/problem/flow_shop.hpp"
#include "fms/problem/module.hpp"
#include "fms/problem/xml_parser.hpp"
#include "fms/solvers/anytime_heuristic.hpp"
#include "fms/solvers/asap_backtrack.hpp"
#include "fms/solvers/asap_cs.hpp"
#include "fms/solvers/branch_bound.hpp"
#include "fms/solvers/broadcast_line_solver.hpp"
#include "fms/solvers/cocktail_line_solver.hpp"
#include "fms/solvers/dd.hpp"
#include "fms/solvers/forward_heuristic.hpp"
#include "fms/solvers/iterated_greedy.hpp"
#include "fms/solvers/mneh_heuristic.hpp"
#include "fms/solvers/pareto_heuristic.hpp"
#include "fms/solvers/partial_solution.hpp"
#include "fms/solvers/sequence.hpp"
#include "fms/solvers/simple.hpp"
#include "fms/versioning.hpp"

#include <chrono>
#include <iostream>

using namespace rapidxml;
using namespace nlohmann;

namespace fms {

void Scheduler::compute(cli::CLIArgs &args) {
    problem::FORPFSSPSDXmlParser parser(args.inputFile);
    switch (parser.getFileType()) {
    case problem::FORPFSSPSDXmlParser::FileType::MODULAR:
        computeModular(args, std::move(parser));
        break;
    case problem::FORPFSSPSDXmlParser::FileType::SHOP: {
        computeShop(args, std::move(parser));
        break;
    }
    }
}

problem::Instance Scheduler::loadFlowShopInstance(cli::CLIArgs &args,
                                                  problem::FORPFSSPSDXmlParser &parser) {
    problem::Instance instance = parser.createFlowShop(args.shopType);

    if (!args.maintPolicyFile.empty()) {
        problem::FORPFSSPSDXmlParser::loadMaintenancePolicy(instance, args.maintPolicyFile);
    }
    return instance;
}

std::pair<bool, std::vector<delay>> Scheduler::checkConsistency(const problem::Instance &flowshop) {
    bool bounds = true; // consistent, unless found otherwise
    const cg::ConstraintGraph &dg = flowshop.getDelayGraph();

    for (const auto &[jobId, ops] : flowshop.jobs()) {
        const auto &prevOp = ops.front();

        for (const auto &op : ops | std::views::drop(1)) {
            // check the intra-job constraints
            if (!dg.hasEdge(op, prevOp)) {
                continue;
            }

            // check intra-job constraints
            cg::Edge minimumSetupTime = dg.getEdge(prevOp, op);
            cg::Edge deadline = dg.getEdge(op, prevOp);
            if (minimumSetupTime.weight + deadline.weight > 0) {
                bounds = false; // deadline cannot be satisfied locally.
                LOG_W(FMT_COMPILE("Deadline between {} and {} cannot be "
                                  "satisfied ({} > {})\n"),
                      prevOp,
                      op,
                      minimumSetupTime.weight,
                      -deadline.weight);
            }
        }
    }

    auto vec = algorithms::paths::initializeASAPST(dg);
    auto result = algorithms::paths::computeASAPST(dg, vec);

    bounds = bounds && result.positiveCycle.empty();
    // earliest possible start times, given no interleavings;
    return {bounds, std::move(vec)};
}

std::tuple<std::vector<solvers::PartialSolution>, nlohmann::json>
Scheduler::runAlgorithm(problem::Instance &flowShopInstance,
                        const cli::CLIArgs &args,
                        const std::uint64_t iteration) {
    nlohmann::json data = {{"algorithm", args.algorithm.shortName()}};

    switch (args.algorithm) {
    case cli::AlgorithmType::ASAP:
        return {{solvers::ASAPCS::solve(flowShopInstance, args)}, std::move(data)};
    case cli::AlgorithmType::ASAP_BACKTRACK:
        return {{solvers::AsapBacktrack::solve(flowShopInstance, args)}, std::move(data)};
    case cli::AlgorithmType::BHCS:
    case cli::AlgorithmType::MIBHCS:
    case cli::AlgorithmType::MISIM:
    case cli::AlgorithmType::MIASAP:
    case cli::AlgorithmType::MIASAPSIM:
        return {{solvers::forward::solve(flowShopInstance, args)}, std::move(data)};
    case cli::AlgorithmType::MDBHCS:
        return {solvers::ParetoHeuristic::solve(flowShopInstance, args), std::move(data)};
    case cli::AlgorithmType::BRANCH_BOUND:
        return {{solvers::branch_bound::solve(flowShopInstance, args)}, std::move(data)};
    case cli::AlgorithmType::ANYTIME:
        return {{solvers::anytime::solve(flowShopInstance, args)}, std::move(data)};
    case cli::AlgorithmType::ITERATED_GREEDY:
        return {{solvers::IteratedGreedy::solve(flowShopInstance, args).solution}, std::move(data)};
    case cli::AlgorithmType::MNEH:
    case cli::AlgorithmType::MNEH_BHCS_COMBI:
    case cli::AlgorithmType::MNEH_BHCS_FLEXIBLE:
    case cli::AlgorithmType::MNEH_ASAP_BACKTRACK:
    case cli::AlgorithmType::MNEH_ASAP:
    case cli::AlgorithmType::MINEH:
    case cli::AlgorithmType::MINEHSIM:
        return {{solvers::MNEH::solve(flowShopInstance, args)}, std::move(data)};
    case cli::AlgorithmType::DD: {
        auto [solutions, tmpData] = solvers::dd::solve(flowShopInstance, args);
        data.update(tmpData);
        return {std::move(solutions), std::move(data)};
    }
    case cli::AlgorithmType::GIVEN_SEQUENCE: {
        auto [solutions, tmpData] = solvers::sequence::solve(flowShopInstance, args, iteration);
        data.update(tmpData);
        return {std::move(solutions), std::move(data)};
    }
    case cli::AlgorithmType::SIMPLE: {
        auto [solutions, tmpData] = solvers::SimpleScheduler::solve(flowShopInstance, args);
        data.update(tmpData);
        return {std::move(solutions), std::move(data)};
    }
    default:
        throw std::runtime_error(
                fmt::format("FmsScheduler::runAlgorithm: algorithm '{}' not supported",
                            args.algorithm.shortName()));
    }
}

std::tuple<std::vector<solvers::PartialSolution>, nlohmann::json>
Scheduler::runAlgorithm(const problem::ProductionLine &problem,
                        problem::Module &flowShopInstance,
                        const cli::CLIArgs &args,
                        const std::uint64_t iteration) {
    const auto algorithm = getAlgorithm(flowShopInstance.getModuleId(),
                                        args.algorithms.size(),
                                        problem.getNumberOfModules(),
                                        args);

    auto argsCopy = args;
    argsCopy.algorithm = algorithm;

    if (algorithm == cli::AlgorithmType::GIVEN_SEQUENCE) {
        // Given sequence behaves differently if it is a module or a normal instance
        return solvers::sequence::solve(flowShopInstance, argsCopy, iteration);
    }
    return runAlgorithm(static_cast<problem::Instance &>(flowShopInstance), argsCopy, iteration);
}

std::tuple<std::vector<solvers::ProductionLineSolution>, nlohmann::json>
Scheduler::runAlgorithm(problem::ProductionLine &problemInstance, const cli::CLIArgs &args) {
    // The modular scheduler may use any algorithm so we cannot call the modular solver
    // within the runAlgorithm method
    switch (args.modularAlgorithm) {
    case cli::ModularAlgorithmType::BROADCAST:
        return solvers::BroadcastLineSolver::solve(problemInstance, args);
    case cli::ModularAlgorithmType::COCKTAIL:
    default:
        return solvers::CocktailLineSolver::solve(problemInstance, args);
    }
}

void Scheduler::saveSolution(const solvers::PartialSolution &solution,
                             const problem::Instance &problem,
                             nlohmann::json &data) {
    LOG("Saving the timing schedule(s) for the scheduling problem");

    nlohmann::json schedule;

    for (const auto &[jobId, ops] : problem.jobs()) {
        for (const auto &op : ops) {
            const auto vId = problem.getDelayGraph().getVertexId(op);
            const auto startTime = solution.getASAPST().at(vId);
            schedule[fmt::to_string(jobId)][fmt::to_string(op.operationId)] = startTime;
        }
    }
    data["schedule"] = schedule;
    data.update(solvers::sequence::saveAllMachinesSequencesTop(
            solution.getChosenSequencesPerMachine()));
}

void Scheduler::saveSolution(const solvers::ProductionLineSolution &solution,
                             const problem::ProductionLine &problem,
                             nlohmann::json &data) {
    nlohmann::json schedule;
    for (const auto &[moduleId, module] : problem.modules()) {
        const auto &moduleSolution = solution[moduleId];
        for (const auto &[jobId, ops] : module.jobs()) {
            for (const auto &op : ops) {
                const auto vId = module.getDelayGraph().getVertexId(op);
                const auto startTime = moduleSolution.getASAPST().at(vId);
                schedule[fmt::to_string(moduleId.value)][fmt::to_string(jobId)]
                        [fmt::to_string(op.operationId)] = startTime;
            }
        }
    }
    data["solution"] = schedule;
    data.update(solvers::sequence::saveProductionLineSequencesTop(solution, problem));
}

void Scheduler::computeShop(cli::CLIArgs &args, problem::FORPFSSPSDXmlParser parser) {
    problem::Instance flowshopInstance = loadFlowShopInstance(args, parser);

    LOG(">> {} SELECTED <<", args.algorithm.description());
    LOG("Solving the scheduling problem instance\n");

    fmt::print(FMT_COMPILE("Solving {}\n"), flowshopInstance.getProblemName());
    solveAndSave(flowshopInstance, args);
}

void Scheduler::computeModular(cli::CLIArgs &args, problem::FORPFSSPSDXmlParser parser) {
    problem::ProductionLine productionLine = parser.createProductionLine(args.shopType);
    LOG(">> {} SELECTED <<", args.modularAlgorithm.shortName());
    solveAndSave(productionLine, args);
}

nlohmann::json Scheduler::initializeData(const cli::CLIArgs &args) {
    return {{"solved", false},
            {"timeout", false},
            {"productivity", args.productivityWeight},
            {"flexibility", args.flexibilityWeight},
            {"timeOutValue", args.timeOut.count()},
            {"version", VERSION}};
}

void Scheduler::saveJSONFile(const nlohmann::json &data, const cli::CLIArgs &args) {
    std::ofstream jsonFile(args.outputFile + ".fms.json", std::ios::out);
    jsonFile << data.dump(4);
    jsonFile.close();
}

void Scheduler::saveCBORFile(const nlohmann::json &data, const cli::CLIArgs &args) {
    std::ofstream cborFile(args.outputFile + ".fms.cbor", std::ios::binary | std::ios::out);
    json::to_cbor(data, cborFile);
    cborFile.close();
}

cli::AlgorithmType Scheduler::getAlgorithm(const problem::ModuleId moduleId,
                                           const std::size_t numAlgorithms,
                                           const std::size_t numModules,
                                           const cli::CLIArgs &args) {
    switch (args.multiAlgorithmBehaviour) {
    case cli::MultiAlgorithmBehaviour::FIRST:
        return args.algorithms.front();
    case cli::MultiAlgorithmBehaviour::DIVIDE: {
        // Distribute modules into groups and assign an algorithm to each group
        const std::size_t groupCount = std::min(numAlgorithms, numModules);
        const std::size_t baseGroupSize = numModules / groupCount;
        const std::size_t remainder = numModules % groupCount;
        const std::size_t index = moduleId.value;

        std::size_t algorithmIndex = 0;
        if (index < remainder * (baseGroupSize + 1)) {
            algorithmIndex = index / (baseGroupSize + 1);
        } else {
            algorithmIndex = remainder + (index - remainder * (baseGroupSize + 1)) / baseGroupSize;
        }

        return args.algorithms.at(algorithmIndex);
    }
    case cli::MultiAlgorithmBehaviour::INTERLEAVE: {
        // Interleave the algorithms for each module
        const std::size_t algIndex = moduleId.value % numAlgorithms;
        return args.algorithms.at(algIndex);
    }
    case cli::MultiAlgorithmBehaviour::LAST:
        return args.algorithms.back();
    case cli::MultiAlgorithmBehaviour::RANDOM:
        return args.algorithms.at(rand() % numAlgorithms);
    }

    throw std::runtime_error(fmt::format("FmsScheduler::getAlgorithm: behaviour '{}' not supported",
                                         args.multiAlgorithmBehaviour.shortName()));
}

} // namespace fms
