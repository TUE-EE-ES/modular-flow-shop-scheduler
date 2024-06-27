#include "pch/containers.hpp"

#include "delayGraph/delayGraph.h"

#include <algorithm>

DelayGraph::VertexID DelayGraph::graph::add_vertex(const FORPFSSPSD::operation &s) {
    VertexID id = lastId++;

    auto &v = vertices.emplace_back(id, s);
    identifierToVertex[s] = id;
    jobToVertex[s.jobId].emplace_back(v.id);
    return id;
}

DelayGraph::Edges DelayGraph::graph::add_edges(const Edges &edges) {
    Edges addedEdges;
    addedEdges.reserve(edges.size());
    for (const auto &e : edges) {
        if (!has_edge(e.src, e.dst)) {
            add_edge(e);
            addedEdges.emplace_back(e);
            // LOG("Actually added to dg egde from {} to {}",e.src,e.dst);
        }
        // else{
        //     LOG("Edge from {} to {} already exists with weight
        //     {}",e.src,e.dst,get_edge(e.src,e.dst).weight);
        // }
    }
    return addedEdges;
}

DelayGraph::VerticesCRef DelayGraph::graph::get_vertices(FORPFSSPSD::JobId jobId) const {
    auto i = jobToVertex.find(jobId);

    if (i == jobToVertex.cend()) {
        throw FmsSchedulerException(fmt::format(
                "Error, unable to find vertices for the given job ({}) in the graph", jobId));
    }

    // Collect references to the vertices
    VerticesCRef vertices;
    vertices.reserve(i->second.size());
    for (const auto &vId : i->second) {
        vertices.emplace_back(get_vertex(vId));
    }

    return vertices;
}

DelayGraph::VerticesRef DelayGraph::graph::get_vertices(FORPFSSPSD::JobId jobId) {
    auto i = jobToVertex.find(jobId);

    if (i == jobToVertex.cend()) {
        throw FmsSchedulerException(fmt::format(
                "Error, unable to find vertices for the given job ({}) in the graph", jobId));
    }

    // Collect references to the vertices
    VerticesRef vertices;
    vertices.reserve(i->second.size());
    for (const auto &vId : i->second) {
        vertices.emplace_back(get_vertex(vId));
    }

    return vertices;
}

DelayGraph::VerticesCRef DelayGraph::graph::cget_vertices() const {
    VerticesCRef result;
    for (const auto &v : vertices) {
        result.push_back(std::cref(v));
    }
    return result;
}

DelayGraph::VerticesCRef DelayGraph::graph::get_vertices(FORPFSSPSD::JobId start_id,
                                                         FORPFSSPSD::JobId end_id) const {
    VerticesCRef result;
    for (auto id = start_id; id <= end_id; ++id) {
        for (const auto &v : get_vertices(id)) {
            result.push_back(std::cref(v));
        }
    }
    return result;
}

DelayGraph::VerticesCRef
DelayGraph::graph::get_vertices(const std::vector<FORPFSSPSD::JobId> &jobIds) const {
    VerticesCRef result;
    for (auto jobId : jobIds) {
        for (const auto &v : get_vertices(jobId)) {
            result.push_back(std::cref(v));
        }
    }
    return result;
}

DelayGraph::VerticesCRef DelayGraph::graph::to_constant(const VerticesRef &vertices) {
    VerticesCRef result;
    for (auto v : vertices) {
        result.push_back(std::cref(v));
    }
    return result;
}

// delayGraph

DelayGraph::VerticesCRef DelayGraph::delayGraph::get_maint_vertices() const {
    VerticesCRef v;
    for (const auto &vert : get_vertices()) {
        if (is_maint(vert)) {
            v.emplace_back(vert);
        }
    }
    return v;
}

DelayGraph::VerticesCRef DelayGraph::delayGraph::get_sources() const {
    VerticesCRef result;
    const auto &vertices = get_vertices();
    std::copy_if(vertices.begin(),
                 vertices.end(),
                 std::back_inserter(result),
                 [&](const vertex &v) { return is_source(v); });
    return result;
}
