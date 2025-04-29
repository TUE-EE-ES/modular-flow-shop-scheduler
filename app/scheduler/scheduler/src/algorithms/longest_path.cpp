#include "fms/pch/containers.hpp"
#include "fms/pch/fmt.hpp"

#include "fms/algorithms/longest_path.hpp"

#include "fms/cg/constraint_graph.hpp"
#include "fms/cg/edge.hpp"
#include "fms/delay.hpp"
#include "fms/problem/operation.hpp"

#include <fstream>
#include <limits>
#include <numeric>

namespace {

template <typename T> fms::problem::JobId minJobId(T &&begin, T &&end) {
    return std::accumulate(begin,
                           end,
                           fms::problem::JobId::max(),
                           [](fms::problem::JobId minJobId, const fms::cg::Vertex &v) {
                               return std::min(v.operation.jobId, minJobId);
                           });
}
} // namespace

namespace fms::algorithms::paths {
using namespace fms::cg;

PathTimes
initializeASAPST(const ConstraintGraph &dg, const VerticesIds &sources, bool graphSources) {
    // Allocate Starting Times array
    PathTimes ASAPST(dg.getNumberOfVertices());
    initializeASAPST(dg, ASAPST, sources, graphSources);
    return std::move(ASAPST);
}

void initializeASAPST(const cg::ConstraintGraph &dg,
                      PathTimes &ASAPST,
                      const cg::VerticesIds &sources,
                      bool graphSources) {

    // Assumption: vertices cannot be deleted, and the id's are continuous
    for (std::size_t i = 0; i < ASAPST.size(); i++) {
        if (dg.isSource(i) && graphSources) {
            ASAPST[i] = 0;
        } else {
            ASAPST[i] = kASAPStartValue;
        }
    }

    for (const auto &source : sources) {
        ASAPST[source] = 0;
    }
}

PathTimes
initializeALAPST(const ConstraintGraph &dg, const VerticesIds & /*sources*/, bool graphSources) {
    // Allocate Starting Times array
    std::vector<delay> ALAPST(dg.getNumberOfVertices());

    // Step 1: Initialize the ASAPST vector.
    // Assumption: vertices cannot be deleted, and the id's are continuous
    for (std::size_t i = 0; i < ALAPST.size(); i++) {
        if (dg.isSource(i) && graphSources) {
            ALAPST[i] = 0;
        } else {
            ALAPST[i] = kALAPStartValue;
        }
    }
    return std::move(ALAPST);
}

void dumpToFile(const ConstraintGraph &dg,
                const std::vector<delay> &ASAPST,
                const std::string &filename) {
    std::ofstream file(filename);
    for (std::size_t i = 0; i < dg.getNumberOfVertices(); i++) {
        fmt::print(file, "[{}] = {}\n", i, dg.getVertex(i).operation, ASAPST[i]);
    }
    file.close();
}

//////////
// ASAP //
//////////

LongestPathResult computeASAPST(const ConstraintGraph &dg, PathTimes &ASAPST) {
    // This version uses the fact that the memory is contiguous to go faster
    for (std::size_t i = 1; i < dg.getNumberOfVertices(); i++) {
        const auto oneEdgeRelaxed = relaxVerticesASAPST(dg, ASAPST);

        // If no relaxation needed to be performed, we should stop.
        if (!oneEdgeRelaxed) {
            return {};
        }
    }

    // Step 3: Check for positive cycles. The nth iteration must not change the values computed in
    // the previous iteration, if that happens, the algorithm will never converge.
    Edges infeasible;
    for (const auto &v : dg.getVertices()) {
        for (const auto &[dst, weight] : v.getOutgoingEdges()) {
            if ((ASAPST[v.id] != kASAPStartValue) && (ASAPST[v.id] + weight) > ASAPST[dst]) {
                infeasible.emplace_back(v.id, dst, weight);
                break;
            }
        }
    }

    return {std::move(infeasible)};
}

//////////
// ALAP //
//////////

LongestPathResult
computeALAPST(const ConstraintGraph &dg, PathTimes &ALAPST, const VerticesIds &sources) {
    // This version uses the fact that the memory is contiguous to go faster
    Edges infeasible;

    for (std::size_t i = 1; i < dg.getNumberOfVertices(); i++) {
        const auto [oneEdgeRelaxed, infeasibleEdge] = relaxVerticesALAPST(dg, ALAPST, sources);

        if (infeasibleEdge) {
            infeasible.push_back(infeasibleEdge.value());
            break;
        }

        // If no relaxation needed to be performed, we should stop.
        if (!oneEdgeRelaxed) {
            break;
        }
    }

    // Step 3: Check for positive cycles. The nth iteration must not change the values computed in
    // the previous iteration, if that happens, the algorithm will never converge.
    for (const auto &v : dg.getVertices()) {
        for (const auto &[src, weight] : v.getIncomingEdges()) {
            if (((ALAPST[v.id] != kALAPStartValue)) && (ALAPST[v.id] - weight) < ALAPST[src]) {
                infeasible.emplace_back(src, v.id, weight);
                break;
            }
        }
    }

    return {std::move(infeasible)};
}

/**
 * Multiple source Bellman-Ford-Moore algorithm to compute longest paths.
 *
 * Iteratively relaxes the incoming edges of the given vertices, to finally arrive at the solution.
 *
 * The starting times of the sources should have been set, and cannot be changed by relaxations, or
 *it is deemed infeasible.
 **/
LongestPathResult computeASAPST(const ConstraintGraph &dg,
                                std::vector<delay> &ASAPST,
                                const VerticesCRef &sources,
                                const VerticesCRef &window) {
    problem::JobId firstJobId = minJobId(window.begin(), window.end());
    Edges infeasible;
    VerticesCRef allVertices{sources};

    const auto &graphSources = dg.getSources();
    allVertices.insert(allVertices.end(), graphSources.begin(), graphSources.end());
    allVertices.insert(allVertices.end(), window.begin(), window.end());

    // For each vertex, iterate once (should be N-1)
    const auto nrVertices = allVertices.size();
    for (std::size_t i = 1; i < nrVertices; i++) {
        // This flag is used to check if the distance of any vertex is relaxed in an iteration.
        // Whenever an edge is relaxed, this flag is set and algorithm continues to next iteration.
        const auto [atLeastOneEdgeRelaxed, infeasibleEdge] =
                relaxVerticesASAPST(allVertices, dg, firstJobId, ASAPST);

        if (infeasibleEdge) {
            infeasible.push_back(infeasibleEdge.value());
            break;
        }

        // If no relaxation needed to be performed, we should stop.
        if (!atLeastOneEdgeRelaxed) {
            break;
        }
    }

    // Step 3: Check for positive cycles. The nth iteration must not change the values computed in
    // the previous iteration, if that happens, the algorithm will never converge.
    for (const Vertex &v : allVertices) {
        for (const auto &[dst, weight] : v.getOutgoingEdges()) {
            // if the vertex was updated at all, check this, otherwise it means the vertex is
            // disconnected from the sources
            if ((ASAPST[v.id] != kASAPStartValue) && ((ASAPST[v.id] + weight) > ASAPST[dst])) {
                infeasible.emplace_back(v.id, dst, weight);
                break;
            }
        }
    }

    return {infeasible};
}

std::tuple<bool, std::optional<Edge>> relaxVerticesASAPST(const VerticesCRef &allVertices,
                                                          const ConstraintGraph &dg,
                                                          problem::JobId firstJobId,
                                                          std::vector<delay> &ASAPST) {

    bool atLeastOneEdgeRelaxed = false;
    for (const Vertex &v : allVertices) {
        for (const auto &[dst, weight] : v.getOutgoingEdges()) {
            auto value = ASAPST[v.id] + weight;
            if (ASAPST[v.id] != kASAPStartValue && value > ASAPST[dst]) {
                if (dg.getVertex(dst).operation.jobId < firstJobId) {
                    // Check if we relaxed a source node (all jobs already scheduled) if we did
                    // then this means that there's a cycle as we shouldn't be able to relax
                    // sources.
                    return {atLeastOneEdgeRelaxed, Edge(v.id, dst, weight)};
                }

                ASAPST[dst] = value;
                atLeastOneEdgeRelaxed = true;
            }
        }
    }

    return {atLeastOneEdgeRelaxed, std::nullopt};
}

bool relaxVerticesASAPST(const ConstraintGraph &dg, PathTimes &ASAPST) {
    bool atLeastOneEdgeRelaxed = false;
    for (const auto &v : dg.getVertices()) {
        for (const auto &[dst, weight] : v.getOutgoingEdges()) {
            auto value = ASAPST[v.id] + weight;
            if (ASAPST[v.id] != kASAPStartValue && value > ASAPST[dst]) {
                ASAPST[dst] = value;
                atLeastOneEdgeRelaxed = true;
            }
        }
    }

    return atLeastOneEdgeRelaxed;
}

std::tuple<bool, std::optional<Edge>>
relaxVerticesALAPST(const ConstraintGraph &dg, PathTimes &ALAPST, const VerticesIds &sources) {
    bool atLeastOneEdgeRelaxed = false;
    for (const auto &v : dg.getVertices()) {
        for (const auto &[src, weight] : v.getIncomingEdges()) {
            auto value = ALAPST[v.id] - weight;
            if (ALAPST[v.id] != kALAPStartValue && value < ALAPST[src]) {
                if (std::find(sources.begin(), sources.end(), src) != sources.end()) {
                    return {atLeastOneEdgeRelaxed,
                            std::nullopt}; // we relaxed a source thus infeasible
                }
                ALAPST[src] = value;
                atLeastOneEdgeRelaxed = true;
            }
        }
    }
    return {atLeastOneEdgeRelaxed, std::nullopt};
}

delay relaxOneEdgeASAPST(const Edge &e, PathTimes &ASAPST) {
    const auto value = ASAPST[e.src] + e.weight;
    if (ASAPST[e.src] != kASAPStartValue && value > ASAPST[e.dst]) {
        const auto relaxAmount = ASAPST[e.dst] == kASAPStartValue
                                         ? std::numeric_limits<delay>::max()
                                         : value - ASAPST[e.dst];
        ASAPST[e.dst] = value;
        return relaxAmount;
    }
    return 0;
}

bool addOneEdgeIncrementalASAPST(const ConstraintGraph &dg, const Edge &e, PathTimes &ASAPST) {
    using VertexPr = std::tuple<delay, VertexId>;
    constexpr auto Comparator = [](const auto &lhs, const auto &rhs) {
        return std::get<0>(lhs) < std::get<0>(rhs);
    };
    std::priority_queue<VertexPr, std::deque<VertexPr>, decltype(Comparator)> toRelax(Comparator);

    if (const auto amount = relaxOneEdgeASAPST(e, ASAPST); amount > 0) {
        toRelax.emplace(amount, e.dst);
    }

    while (!toRelax.empty()) {
        const auto [_, v] = toRelax.top();
        toRelax.pop();

        for (const auto &[dst, weight] : dg.getVertex(v).getOutgoingEdges()) {
            if (const auto amount = relaxOneEdgeASAPST({v, dst, weight}, ASAPST); amount > 0) {
                toRelax.emplace(amount, dst);
            }
        }

        if (v == e.src && relaxOneEdgeASAPST(e, ASAPST) > 0) {
            // We relaxed the edge we added thus, we found a positive cycle.
            return true;
        }
    }

    return false;
}

bool addEdgesIncrementalASAPST(ConstraintGraph &dg, const Edges &edges, PathTimes &ASAPST) {
    Edges addedEdges;

    for (const auto &e : edges) {
        if (addOneEdgeIncrementalASAPST(dg, e, ASAPST)) {
            dg.removeEdges(addedEdges);
            return true;
        }

        if (!dg.hasEdge(e)) {
            dg.addEdges(e);
            addedEdges.push_back(e);
        }
    }

    dg.removeEdges(addedEdges);
    return false;
}

bool addEdgesIncrementalASAPSTConst(ConstraintGraph dg, const Edges &edges, PathTimes &ASAPST) {
    for (const auto &e : edges) {
        if (addOneEdgeIncrementalASAPST(dg, e, ASAPST)) {
            return true;
        }

        if (!dg.hasEdge(e)) {
            dg.addEdges(e);
        }
    }
    return false;
}

std::vector<Edge> getPositiveCycle(const ConstraintGraph &dg) {
    // Code mostly from https://cp-algorithms.com/graph/finding-negative-cycle-in-graph.html

    auto ASAPST = initializeASAPST(dg);
    const auto &vertices = dg.getVertices();

    std::vector<std::optional<VertexId>> previous(vertices.size());
    std::optional<VertexId> lastModified;

    // First find an edge that is part of the cycle
    for (std::size_t i = 0; i < vertices.size(); i++) {
        lastModified = std::nullopt;
        for (const auto &v : vertices) {
            for (const auto &[dst, weight] : v.getOutgoingEdges()) {
                const auto value = ASAPST[v.id] + weight;
                if (ASAPST[v.id] != kASAPStartValue && value > ASAPST[dst]) {
                    ASAPST[dst] = value;
                    previous[dst] = v.id;
                    lastModified = dst;
                }
            }
        }
    }

    // Now, follow the cycle for n iterations, if at some point lastModified is not set,
    // then, there's no cycle.
    for (std::size_t i = 0; i < vertices.size(); i++) {
        if (!lastModified.has_value()) {
            return {};
        }
        lastModified = previous[lastModified.value()];
    }

    if (!lastModified.has_value()) {
        return {};
    }

    VertexId vLast = lastModified.value();
    std::vector<Edge> cycle;
    bool first = true;

    for (VertexId v = vLast;; v = *previous[v]) {
        if (v == vLast && cycle.size() > 1) {
            break;
        }

        VertexId src = *previous[v];
        cycle.emplace_back(src, v, dg.getVertex(src).getWeight(v));
    }

    return cycle;
}

} // namespace fms::algorithms::paths
