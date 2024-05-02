#ifndef SOLVERS_UTILS_HPP
#define SOLVERS_UTILS_HPP

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "delayGraph/delayGraph.h"
#include "longest_path.h"
#include "partialsolution.h"

namespace algorithm {
class SolversUtils {
public:
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
    static std::tuple<LongestPathResult, PathTimes>
    checkSolutionAndOutputIfFails(const std::string &fileName,
                                  DelayGraph::delayGraph &dg,
                                  const DelayGraph::Edges &extraEdges = {},
                                  const std::string &extraMessage = "");

    static std::tuple<LongestPathResult, PathTimes>
    checkSolutionAndOutputIfFails(const FORPFSSPSD::Instance &instance);

    static std::tuple<LongestPathResult, PathTimes>
    checkSolutionAndOutputIfFails(const FORPFSSPSD::Instance &instance, PartialSolution &ps);

    static void checkPathResultAndOutputIfFails(const std::string &fileName,
                                                const DelayGraph::delayGraph &dg,
                                                const std::string &extraMessage,
                                                const algorithm::LongestPathResult &result,
                                                const DelayGraph::Edges &extraEdges = {});

    static PartialSolution createTrivialSolution(FORPFSSPSD::Instance &problem);

    static DelayGraph::Edges createMachineTrivialSolution(const FORPFSSPSD::Instance &problem,
                                                          FORPFSSPSD::MachineId machineId);
};
} // namespace algorithm

#endif