#ifndef SOLVERS_UTILS_HPP
#define SOLVERS_UTILS_HPP

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "FORPFSSPSD/indices.hpp"
#include "delayGraph/delayGraph.h"
#include "longest_path.h"
#include "partialsolution.h"

#include <string>
#include <tuple>

namespace algorithm::SolversUtils {
/**
 * @brief Checks if the given graph is feasible and throws and saves it to disk if it is not.
 *
 * If the graph is feasible (contains positive cycles), no output graph file is created.
 *
 * @param fileName File name of the output file to save without extension.
 * @param dg Graph to check
 * @param extraEdges Extra edges to add to the graph before checking.
 * @param extraMessage Message that will be displayed when throwing an exception if the graph is
 * infeasible.
 */
std::tuple<LongestPathResult, PathTimes>
checkSolutionAndOutputIfFails(const std::string &fileName,
                              DelayGraph::delayGraph &dg,
                              const DelayGraph::Edges &extraEdges = {},
                              const std::string &extraMessage = "");

std::tuple<LongestPathResult, PathTimes>
checkSolutionAndOutputIfFails(const FORPFSSPSD::Instance &instance);

std::tuple<LongestPathResult, PathTimes>
checkSolutionAndOutputIfFails(const FORPFSSPSD::Instance &instance, PartialSolution &ps);

void checkPathResultAndOutputIfFails(const std::string &fileName,
                                     const DelayGraph::delayGraph &dg,
                                     const std::string &extraMessage,
                                     const algorithm::LongestPathResult &result,
                                     const DelayGraph::Edges &extraEdges = {});

PartialSolution createTrivialSolution(FORPFSSPSD::Instance &problem);

DelayGraph::Edges createMachineTrivialSolution(const FORPFSSPSD::Instance &problem,
                                               FORPFSSPSD::MachineId machineId);

/**
 * @brief Creates the initial graph for the problem instance if it is not already initialized.
 *
 * If the graph is infeasible, an exception is thrown and the graph is saved as a DOT file
 * with the name `input_infeasible_<problemName>.dot`.
 *
 * @param problemInstance Problem to generate the graph for.
 * @param saveGraph If true, the graph will be saved as a DOT file named
 * `input_graph_<problemName>.dot`. Used for debugging.
 * @return PathTimes Earliest start times for the operations in the graph.
 */
PathTimes initProblemGraph(FORPFSSPSD::Instance &problemInstance, bool saveGraph = false);
} // namespace algorithm::SolversUtils

#endif