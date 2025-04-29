#ifndef FMS_SOLVERS_UTILS_HPP
#define FMS_SOLVERS_UTILS_HPP

#include "fms/algorithms/longest_path.hpp"
#include "fms/cg/constraint_graph.hpp"
#include "fms/problem/flow_shop.hpp"
#include "fms/problem/indices.hpp"

#include "partial_solution.hpp"

#include <string>

namespace fms::solvers::SolversUtils {
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
algorithms::paths::LongestPathResultWithTimes
checkSolutionAndOutputIfFails(const std::string &fileName,
                              cg::ConstraintGraph &dg,
                              const cg::Edges &extraEdges = {},
                              const std::string &extraMessage = "");

algorithms::paths::LongestPathResultWithTimes
checkSolutionAndOutputIfFails(const problem::Instance &instance);

algorithms::paths::LongestPathResultWithTimes
checkSolutionAndOutputIfFails(const problem::Instance &instance, PartialSolution &ps);

void checkPathResultAndOutputIfFails(const std::string &fileName,
                                     const cg::ConstraintGraph &dg,
                                     const std::string &extraMessage,
                                     const algorithms::paths::LongestPathResultWithTimes &result,
                                     const cg::Edges &extraEdges = {});

PartialSolution createTrivialSolution(problem::Instance &problem);

Sequence createMachineTrivialSolution(const problem::Instance &problem,
                                      problem::MachineId machineId);

Sequence getInferredInputSequence(const problem::Instance &problem,
                                  const MachinesSequences &sequences);
Sequence getInferredInputSequence(const problem::Instance &problem,
                                  const Sequence &firstReEntrantMachineSequence);

void printSequence(const Sequence &sequence);
void printSequenceIfDebug(const Sequence &sequence);

cg::Edges getEdgesFromSequence(const problem::Instance &problem,
                               const Sequence &sequence,
                               problem::MachineId machineId);

cg::Edges getAllEdgessFromSequences(const problem::Instance &problem,
                                    const MachinesSequences &sequences);
cg::Edges getInferredEdges(const problem::Instance &problem, const MachinesSequences &sequences);
cg::Edges getInferredEdges(const problem::Instance &problem,
                           const Sequence &firstReEntrantMachineSequence);
cg::Edges getInferredEdgesFromInferredSequence(const problem::Instance &problem,
                                               const Sequence &inferredSequence);
cg::Edges getAllEdgesPlusInferredEdges(const problem::Instance &problem,
                                       const MachinesSequences &sequences);
cg::Edges getAllEdgesPlusInferredEdges(const problem::Instance &problem,
                                       const Sequence &firstReEntrantMachineSequence);

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
algorithms::paths::PathTimes initProblemGraph(problem::Instance &problemInstance,
                                              bool saveGraph = false);
} // namespace fms::solvers::SolversUtils

#endif
