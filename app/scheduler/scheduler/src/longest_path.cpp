#include "longest_path.h"

#include "FORPFSSPSD/operation.h"
#include "delay.h"
#include "delayGraph/delayGraph.h"
#include "delayGraph/edge.h"
#include "negativecyclefinder.h"

#include <fmt/ostream.h>
#include <fstream>
#include <iostream>
#include <limits>
#include <numeric>

using namespace DelayGraph;
using namespace algorithm;

namespace {

template <typename T> FORPFSSPSD::JobId minJobId(T &&begin, T &&end) {
    return std::accumulate(begin,
                           end,
                           std::numeric_limits<FORPFSSPSD::JobId>::max(),
                           [](FORPFSSPSD::JobId minJobId, const vertex &v) {
                               return std::min(v.operation.jobId, minJobId);
                           });
}
} // namespace

std::vector<delay>
LongestPath::initializeASAPST(const delayGraph &dg, const VerticesIDs &sources, bool graphSources) {
    // Allocate Starting Times array
    std::vector<delay> ASAPST(dg.get_number_of_vertices());

    // Step 1: Initialize the ASAPST vector.
    // Assumption: vertices cannot be deleted, and the id's are continuous
    for (std::size_t i = 0; i < ASAPST.size(); i++) {
        if (dg.is_source(i) && graphSources) {
            ASAPST[i] = 0;
        } else {
            ASAPST[i] = kASAPStartValue;
        }
    }

    for (const auto &source : sources) {
        ASAPST[source] = 0;
    }

    return std::move(ASAPST);
}

std::vector<delay> LongestPath::initializeALAPST(const DelayGraph::delayGraph &dg,
                                                 const DelayGraph::VerticesIDs & /*sources*/,
                                                 bool graphSources) {
    // Allocate Starting Times array
    std::vector<delay> ALAPST(dg.get_number_of_vertices());

    // Step 1: Initialize the ASAPST vector.
    // Assumption: vertices cannot be deleted, and the id's are continuous
    for (std::size_t i = 0; i < ALAPST.size(); i++) {
        if (dg.is_source(i) && graphSources) {
            ALAPST[i] = 0;
        } else {
            ALAPST[i] = kALAPStartValue;
        }
    }
    return std::move(ALAPST);
}

void LongestPath::dumpToFile(const delayGraph &dg,
                             const std::vector<delay> &ASAPST,
                             const std::string &filename) {
    std::ofstream file(filename);
    for (std::size_t i = 0; i < dg.get_number_of_vertices(); i++) {
        fmt::print(file, "[{}] = {}\n", i, dg.get_vertex(i).operation, ASAPST[i]);
    }
    file.close();
}

//////////
// ASAP //
//////////

algorithm::LongestPathResult algorithm::LongestPath::computeASAPST(const DelayGraph::delayGraph &dg,
                                                                   PathTimes &ASAPST) {
    // This version uses the fact that the memory is contiguous to go faster
    for (std::size_t i = 1; i < dg.get_number_of_vertices(); i++) {
        const auto oneEdgeRelaxed = relaxVerticesASAPST(dg, ASAPST);

        // If no relaxation needed to be performed, we should stop.
        if (!oneEdgeRelaxed) {
            return {};
        }
    }

    // Step 3: Check for positive cycles. The nth iteration must not change the values computed in
    // the previous iteration, if that happens, the algorithm will never converge.
    DelayGraph::Edges infeasible;
    for (const vertex &v : dg.get_vertices()) {
        for (const auto &[dst, weight] : v.get_outgoing_edges()) {
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

algorithm::LongestPathResult algorithm::LongestPath::computeALAPST(const DelayGraph::delayGraph &dg,
                                                                   PathTimes &ALAPST,
                                                                   const VerticesIDs &sources) {
    // This version uses the fact that the memory is contiguous to go faster
    DelayGraph::Edges infeasible;

    for (std::size_t i = 1; i < dg.get_number_of_vertices(); i++) {
        const auto [oneEdgeRelaxed, infeasibleEdge] = relaxVerticesALAPST(dg, ALAPST,sources);

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
    for (const vertex &v : dg.get_vertices()) {
        for (const auto &[src, weight] : v.get_incoming_edges()) {
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
LongestPathResult LongestPath::computeASAPST(const DelayGraph::delayGraph &dg,
                                             std::vector<delay> &ASAPST,
                                             const DelayGraph::VerticesCRef &sources,
                                             const DelayGraph::VerticesCRef &window) {
    FORPFSSPSD::JobId firstJobId = minJobId(window.begin(), window.end());
    DelayGraph::Edges infeasible;
    VerticesCRef allVertices{sources};

    const auto &graphSources = dg.get_sources();
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
    for (const vertex &v : allVertices) {
        for (const auto &[dst, weight] : v.get_outgoing_edges()) {
            //if the vertex was updated at all, check this, otherwise it means the vertex is disconnected from the sources
            if ((ASAPST[v.id] != kASAPStartValue) && ((ASAPST[v.id] + weight) > ASAPST[dst])) {
                infeasible.emplace_back(v.id, dst, weight);
                break;
            }
        }
    }

    return {infeasible};
}

std::tuple<bool, std::optional<DelayGraph::edge>>
algorithm::LongestPath::relaxVerticesASAPST(const VerticesCRef &allVertices,
                                            const DelayGraph::delayGraph &dg,
                                            FORPFSSPSD::JobId firstJobId,
                                            std::vector<delay> &ASAPST) {

    bool atLeastOneEdgeRelaxed = false;
    for (const vertex &v : allVertices) {
        for (const auto &[dst, weight] : v.get_outgoing_edges()) {
            auto value = ASAPST[v.id] + weight;
            if (ASAPST[v.id] != kASAPStartValue && value > ASAPST[dst]) {
                if (dg.get_vertex(dst).operation.jobId < firstJobId) {
                    // Check if we relaxed a source node (all jobs already scheduled) if we did
                    // then this means that there's a cycle as we shouldn't be able to relax
                    // sources.
                    return {atLeastOneEdgeRelaxed, edge(v.id, dst, weight)};
                }

                ASAPST[dst] = value;
                atLeastOneEdgeRelaxed = true;
            }
        }
    }

    return {atLeastOneEdgeRelaxed, std::nullopt};
}

bool
algorithm::LongestPath::relaxVerticesASAPST(const DelayGraph::delayGraph &dg, PathTimes &ASAPST) {
    bool atLeastOneEdgeRelaxed = false;
    for (const vertex &v : dg.get_vertices()) {
        for (const auto &[dst, weight] : v.get_outgoing_edges()) {
            auto value = ASAPST[v.id] + weight;
            if (ASAPST[v.id] != kASAPStartValue && value > ASAPST[dst]) {
                ASAPST[dst] = value;
                atLeastOneEdgeRelaxed = true;
            }
        }
    }

    return atLeastOneEdgeRelaxed;
}


std::tuple<bool, std::optional<DelayGraph::edge>>
algorithm::LongestPath::relaxVerticesALAPST(const DelayGraph::delayGraph &dg, PathTimes &ALAPST,const VerticesIDs &sources) {
    bool atLeastOneEdgeRelaxed = false;
    for (const vertex &v : dg.get_vertices()) {
        for (const auto &[src, weight] : v.get_incoming_edges()) {
            auto value = ALAPST[v.id] - weight;
            if (ALAPST[v.id] != kALAPStartValue && value < ALAPST[src]) {
                if(std::find(sources.begin(),sources.end(),src) != sources.end()){
                    return {atLeastOneEdgeRelaxed, std::nullopt}; // we relaxed a source thus infeasible
                }
                ALAPST[src] = value;
                atLeastOneEdgeRelaxed = true;
            }
        }
    }
    return {atLeastOneEdgeRelaxed, std::nullopt};
}

delay algorithm::LongestPath::relaxOneEdgeASAPST(const DelayGraph::edge &e, PathTimes &ASAPST) {
    const auto value = ASAPST[e.src] + e.weight;
    if (ASAPST[e.src] != kASAPStartValue && value > ASAPST[e.dst]) {
        const auto relaxAmount = ASAPST[e.dst] == kASAPStartValue ? std::numeric_limits<delay>::max()
                                                              : value - ASAPST[e.dst];
        ASAPST[e.dst] = value;
        return relaxAmount;
    }
    return 0;
}

bool algorithm::LongestPath::addOneEdgeIncrementalASAPST(const DelayGraph::delayGraph &dg,
                                                         const DelayGraph::edge &e,
                                                         PathTimes &ASAPST) {
    using VertexPr = std::tuple<delay, VertexID>;
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

        for (const auto &[dst, weight] : dg.get_vertex(v).get_outgoing_edges()) {
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

bool algorithm::LongestPath::addEdgesIncrementalASAPST(DelayGraph::delayGraph &dg,
                                                       const DelayGraph::Edges &edges,
                                                       PathTimes &ASAPST) {
    DelayGraph::Edges addedEdges;

    for (const auto &e : edges) {
        if (addOneEdgeIncrementalASAPST(dg, e, ASAPST)) {
            dg.remove_edges(addedEdges);
            return true;
        }

        if (!dg.has_edge(e)) {
            dg.add_edge(e);
            addedEdges.push_back(e);
        }
    }

    dg.remove_edges(addedEdges);
    return false;
}

bool algorithm::LongestPath::addEdgesIncrementalASAPSTConst(DelayGraph::delayGraph dg,
                                                            const DelayGraph::Edges &edges,
                                                            PathTimes &ASAPST) {
    for (const auto &e : edges) {
        if (addOneEdgeIncrementalASAPST(dg, e, ASAPST)) {
            return true;
        }

        if (!dg.has_edge(e)) {
            dg.add_edge(e);
        }
    }
    return false;
}

std::vector<DelayGraph::edge>
algorithm::LongestPath::getPositiveCycle(const DelayGraph::delayGraph &dg) {
    // Code mostly from https://cp-algorithms.com/graph/finding-negative-cycle-in-graph.html

    auto ASAPST = initializeASAPST(dg);
    const auto &vertices = dg.get_vertices();

    std::vector<std::optional<VertexID>> previous(vertices.size());
    std::optional<VertexID> lastModified;

    // First find an edge that is part of the cycle
    for (std::size_t i = 0; i < vertices.size(); i++) {
        lastModified = std::nullopt;
        for (const vertex &v : vertices) {
            for (const auto &[dst, weight] : v.get_outgoing_edges()) {
                const auto value = ASAPST[v.id] + weight;
                if (ASAPST[v.id] != kASAPStartValue && value > ASAPST[dst]) {
                    ASAPST[dst] = value;
                    previous[dst] = v.id;
                    lastModified = dst;
                }
            }
        }
    }

    if (!lastModified.has_value()) {
        return {};
    }

    // Now, follow the cycle for n iterations
    for (std::size_t i = 0; i < vertices.size(); i++) {
        if (!lastModified.has_value()) {
            return {};
        }
        lastModified = previous[lastModified.value()];
    }

    if (!lastModified.has_value()) {
        return {};
    }

    VertexID vLast = lastModified.value();
    std::vector<DelayGraph::edge> cycle;
    bool first = true;

    for (VertexID v = vLast;; v = *previous[v]) {
        if (first) {
            // We don't want to add the last edge twice
            first = false;
        } else {
            VertexID src = *previous[v];
            cycle.emplace_back(src, v, dg.get_vertex(src).get_weight(v));
        }

        if (v == vLast && cycle.size() > 1) {
            break;
        }
    }

    return cycle;
}
