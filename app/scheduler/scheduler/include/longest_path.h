#ifndef LONGEST_PATH_H
#define LONGEST_PATH_H

#include "delay.h"
#include "delayGraph/delayGraph.h"
#include "delayGraph/edge.h"

#include <limits>
#include <utility>

/*
 * Compute the longest path between a source vertex and any vertex in a graph, if no positive cycles
 * exist.
 *
 * Each implementation of computeASAPST takes a list of ASAPST that has at least as many entries as
 * there are nodes in the graph The implementation also modifies the content of the ASAPST list, and
 * returns a negative cycle when at least one exists, or an empty vector otherwise.
 *
 * If a negative cycle exists, the Belman-Ford-Moore algorithm could not converge, and the graph
 * does not have ASAPST defined. In addition; if an edge must be relaxed that is BEFORE the current
 * window, the re-timing is not allowed, and it is considered infeasible as well
 */
namespace algorithm {

using PathTimes = std::vector<delay>;

struct LongestPathResult {
    const DelayGraph::Edges positiveCycle;
};

class LongestPath {
public:
    using PathFunction = LongestPathResult (&)(const DelayGraph::delayGraph &, PathTimes &);

    /// Starting value of ASAP computation. Equivalent to @f$-\infty@f$
    static constexpr delay kASAPStartValue = std::numeric_limits<delay>::min();

    /// Starting value of ALAP computation. Equivalent to @f$+\infty@f$
    static constexpr delay kALAPStartValue = std::numeric_limits<delay>::max();

    /* Compute the ASAP start time by using Belman-Ford-Moore algorithm. The computation complexity
     * is O(VE). */

    /**
     * @brief Compute earliest start times
     *
     * Use the Bellman-Ford longest path algorithm com compute the As Soon As Possible start times
     * of the constraint-graph @p dg and store them in @p ASAPST. @p ASAPST must be initialized
     * using @ref initializeASAPST. The computation complexity is @f$O(VE)@f$.
     *
     * @param[in] dg Graph to evaluate
     * @param[in,out] ASAPST Initialized starting times that will be updated with the ASAP.
     * @return LongestPathResult
     */
    static LongestPathResult computeASAPST(const DelayGraph::delayGraph &dg, PathTimes &ASAPST);

    /**
     * @brief Overload of @ref computeASAPST
     * @details This overload adds the @p inputEdges to the
     * graph before computing the ASAP times and removes them after it finishes.
     * @param inputEdges Edges to be added to the graph before computing ASAPST
     */
    inline static LongestPathResult computeASAPST(DelayGraph::delayGraph &dg,
                                                  PathTimes &ASAPST,
                                                  const DelayGraph::Edges &inputEdges) {
        DelayGraph::Edges edges = dg.add_edges(inputEdges);
        auto result = LongestPath::computeASAPST(dg, ASAPST);
        dg.remove_edges(edges);
        return result;
    }

    /**
     * @brief Overload of @ref computeASAPST
     * @details This overload only applies the BF algorithm to the
     * subset of the graph that contains the vertices of @p sources, @p window, and the graph
     * sources defined with @ref DelayGraph::delayGraph::add_source.
     *
     * @note @p sources and @p window should only be one parameter (as they are merged) but
     * they are kept separate for historical reasons because there's a lot of code that uses both.
     * @param sources Subset of vertices to consider of the graph for longest-path computation.
     * @param window Window of the graph to consider for longest-path computation.
     */
    static LongestPathResult computeASAPST(const DelayGraph::delayGraph &dg,
                                           PathTimes &ASAPST,
                                           const DelayGraph::VerticesCRef &sources,
                                           const DelayGraph::VerticesCRef &window);

    /**
     * @brief Overload of @ref computeASAPST
     * @details This overload does not require the initialized starting times as it creates them 
     * from the sources provided in @p sources and the graph sources (@ref 
     * DelayGraph::delayGraph::add_source) if @p graphSources is _true_.
     * @param sources Vertices to initialize as sources (setting distance to 0)
     * @param graphSources If _true_ the graph sources will be used. Otherwise only the @p sources
     * will be used.
     * @return std::tuple<LongestPathResult, PathTimes>
     */
    [[nodiscard]] inline static std::tuple<LongestPathResult, PathTimes>
    computeASAPST(const DelayGraph::delayGraph &dg,
                  const DelayGraph::VerticesIDs &sources = {},
                  bool graphSources = true) {
        auto ASAPST = initializeASAPST(dg, sources, graphSources);
        auto result = computeASAPST(dg, ASAPST);
        return {std::move(result), std::move(ASAPST)};
    }

    /**
     * @brief Overload of @ref computeASAPST
     * @details This overload does not require the initialized starting times as it creates them 
     * from the @p sources and the graph sources (@ref DelayGraph::delayGraph::add_source) if @p
     * graphSources is _true_. Moreover, it adds the extra @p edges to the graph before computing
     * the longest path.
     * @param dg Graph
     * @param edges Extra edges to be added to the graph befor computing longest path
     * @param sources Vertices to consider as sources and initialize at distance 0
     * @param graphSources If _true_ the graph sources will also be used to initialize the longest
     * path times
     * @return 
     */
    [[nodiscard]] inline static std::tuple<LongestPathResult, PathTimes>
    computeASAPST(DelayGraph::delayGraph &dg,
                  const DelayGraph::Edges &edges,
                  const DelayGraph::VerticesIDs &sources = {},
                  bool graphSources = true) {
        auto ASAPST = initializeASAPST(dg, sources, graphSources);
        return {computeASAPST(dg, ASAPST, edges), std::move(ASAPST)};
    }

    /**
     * @brief Computes the longest path from a single node
     * @details Computes the distance from the node @p source to all the other nodes in the graph.
     * @param dg Graph
     * @param source Source node to compute the longest path from
     * @param edges Extra edges to be added to the graph before computing longest path
     * @return PathTimes Length of the longest path from @p source to all the other nodes. If the 
     * length is equal to @ref kASAPStartValue it means that the node is not reachable from @p
     * source.
     */
    [[nodiscard]] inline static PathTimes
    computeASAPSTFromNode(DelayGraph::delayGraph &dg,
                          DelayGraph::VertexID source,
                          const DelayGraph::Edges &edges = {}) {
        auto ASAPST = initializeASAPST(dg, {source}, false);
        computeASAPST(dg, ASAPST, edges);
        return ASAPST;
    }

    [[nodiscard]] static LongestPathResult
    computeALAPST(const DelayGraph::delayGraph &dg,
                  PathTimes &ALAPST,
                  const DelayGraph::VerticesIDs &sources = {});

    [[nodiscard]] static std::tuple<LongestPathResult, PathTimes> inline computeALAPST(
            const DelayGraph::delayGraph &dg, const DelayGraph::VerticesIDs &sources = {}) {
        auto ALAPST = initializeALAPST(dg, sources);
        return {computeALAPST(dg, ALAPST, sources), std::move(ALAPST)};
    }

    /**
     * @brief Initialize the longest path times by setting 0 to the sources and
     * @f$-\infty@f$ to the other vertices.
     *
     * @param dg Graph
     * @param sources List of vertices to set at 0
     * @param graphSources If true, the sources of the graph are used additionally to
     * @p sources .
     * @return PathTimes Initialized vector of longest paths from the sources to the
     * vertices.
     */
    static PathTimes initializeASAPST(const DelayGraph::delayGraph &dg,
                                      const DelayGraph::VerticesIDs &sources = {},
                                      bool graphSources = true);

    /**
     * @overload
     */
    static PathTimes initializeALAPST(const DelayGraph::delayGraph &dg, 
                                        const DelayGraph::VerticesIDs &sources = {}, 
                                        bool graphSources = true);

    static DelayGraph::Edges findPositiveCycle(const DelayGraph::delayGraph &dg,
                                               const DelayGraph::Edges &edgeTo);

    static void dumpToFile(const DelayGraph::delayGraph &dg,
                           const PathTimes &ASAPST,
                           const std::string &filename);

    static std::tuple<bool, std::optional<DelayGraph::edge>>
    relaxVerticesASAPST(const DelayGraph::VerticesCRef &allVertices,
                        const DelayGraph::delayGraph &dg,
                        FORPFSSPSD::JobId firstJobId,
                        PathTimes &ASAPST);
    static bool relaxVerticesASAPST(const DelayGraph::delayGraph &dg, PathTimes &ASAPST);

    static std::tuple<bool, std::optional<DelayGraph::edge>>
    relaxVerticesALAPST(const DelayGraph::delayGraph &dg, PathTimes &ALAPST,const DelayGraph::VerticesIDs &sources);

    /**
     * @brief Relaxes one edge and returns true if the relaxation was successful
     *
     * @param e Edge to relax
     * @param ASAPST Longest path times for each vertex
     * @return delay Amount that the destination vertex was relaxed. If no relaxation
     * the result is 0.
     */
    static delay relaxOneEdgeASAPST(const DelayGraph::edge &e, PathTimes &ASAPST);

    /**
     * @brief Incremental check of positive cycles with one edge
     * 
     * Checks if adding one edge @p e to the graph @p dg would create a
     * positive cycle while knowing the existing longest path times @p ASAPST .
     *
     * @param dg Graph
     * @param e Edge to add to the graph
     * @param ASAPST Known longest path times of @p dg
     * @return _true_ If adding @p e to @p dg would create a positive cycle, otherwise _false_.
     */
    static bool addOneEdgeIncrementalASAPST(const DelayGraph::delayGraph &dg,
                                            const DelayGraph::edge &e,
                                            PathTimes &ASAPST);

    /**
     * @brief Incremental check of positive cycles with multiple edges
     *
     * This function checks if adding the edges @p edges to the graph @p dg would create positive
     * cycles in an incremental way.
     * @param dg Graph
     * @param edges Edges to add to the graph
     * @param ASAPST Known longest path times of @p dg
     * @return _true_ If adding the edges @p edges to the graph would create a positive cycle, 
     * otherwise _false_.
     */
    static bool addEdgesIncrementalASAPST(DelayGraph::delayGraph &dg,
                                          const DelayGraph::Edges &edges,
                                          PathTimes &ASAPST);

    /**
     * @copydoc addEdgesIncrementalASAPST(DelayGraph::delayGraph&, const DelayGraph::Edges&, 
     * PathTimes&)
     * @note This version does not modify the original graph but uses a copy of it.
     */
    static bool addEdgesIncrementalASAPSTConst(DelayGraph::delayGraph dg,
                                               const DelayGraph::Edges &edges,
                                               PathTimes &ASAPST);

    /**
     * @brief Checks whether adding the edges is successful and doesn't add a positive cycle.
     * 
     * @param dg Delay graph to check
     * @param edges Extra edges to add to the graph
     * @param ASAPST Current start times
     * @return true Adding the edges does not cause a positive cycle
     * @return false Adding the edges causes a positive cycle
     */
    inline static bool addEdgesSuccessful(DelayGraph::delayGraph &dg,
                                          const DelayGraph::Edges &edges,
                                          PathTimes &ASAPST) {
        return computeASAPST(dg, ASAPST, edges).positiveCycle.empty();
    }

    /**
     * @brief Finds the positive cycle in the given delay graph.
     * 
     * @param dg The delay graph to search for the positive cycle.
     * @return The positive cycle found in the delay graph, if any.
     */
    [[nodiscard]] static std::vector<DelayGraph::edge>
    getPositiveCycle(const DelayGraph::delayGraph &dg);

    [[nodiscard]] inline static auto getPositiveCycle(DelayGraph::delayGraph &dg,
                                                      const DelayGraph::Edges &edges) {
        const auto addedEdges = dg.add_edges(edges);
        auto result = getPositiveCycle(dg);
        dg.remove_edges(addedEdges);
        return result;
    }
};
} // namespace algorithm
#endif // LONGEST_PATH_H
