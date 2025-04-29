#include "fms/pch/containers.hpp"

#include "fms/cg/constraint_graph.hpp"

#include <algorithm>

using namespace fms;

cg::VertexId cg::Graph::addVertex(const problem::Operation &s) {
    VertexId id = lastId++;

    auto &v = vertices.emplace_back(id, s);
    identifierToVertex[s] = id;
    jobToVertex[s.jobId].emplace_back(v.id);
    return id;
}

cg::Edges cg::Graph::addEdges(const Edges &edges) {
    Edges addedEdges;
    addedEdges.reserve(edges.size());
    for (const auto &e : edges) {
        if (!hasEdge(e.src, e.dst)) {
            addEdges(e);
            addedEdges.emplace_back(e);
            // LOG("Actually added to dg egde from {} to {}",e.src,e.dst);
        }
        // else{
        //     LOG("Edge from {} to {} already exists with weight
        //     {}",e.src,e.dst,getEdge(e.src,e.dst).weight);
        // }
    }
    return addedEdges;
}

cg::VerticesCRef cg::Graph::getVertices(problem::JobId jobId) const {
    auto i = jobToVertex.find(jobId);

    if (i == jobToVertex.cend()) {
        throw FmsSchedulerException(fmt::format(
                "Error, unable to find vertices for the given job ({}) in the graph", jobId));
    }

    // Collect references to the vertices
    VerticesCRef vertices;
    vertices.reserve(i->second.size());
    for (const auto &vId : i->second) {
        vertices.emplace_back(getVertex(vId));
    }

    return vertices;
}

cg::VerticesRef cg::Graph::getVertices(problem::JobId jobId) {
    auto i = jobToVertex.find(jobId);

    if (i == jobToVertex.cend()) {
        throw FmsSchedulerException(fmt::format(
                "Error, unable to find vertices for the given job ({}) in the graph", jobId));
    }

    // Collect references to the vertices
    VerticesRef vertices;
    vertices.reserve(i->second.size());
    for (const auto &vId : i->second) {
        vertices.emplace_back(getVertex(vId));
    }

    return vertices;
}

cg::VerticesCRef cg::Graph::getVerticesC() const {
    VerticesCRef result;
    for (const auto &v : vertices) {
        result.push_back(std::cref(v));
    }
    return result;
}

cg::VerticesCRef cg::Graph::getVertices(problem::JobId start_id,
                                                         problem::JobId end_id) const {
    VerticesCRef result;
    for (auto id = start_id; id <= end_id; ++id) {
        for (const auto &v : getVertices(id)) {
            result.push_back(std::cref(v));
        }
    }
    return result;
}

cg::VerticesCRef
cg::Graph::getVertices(const std::vector<problem::JobId> &jobIds) const {
    VerticesCRef result;
    for (auto jobId : jobIds) {
        for (const auto &v : getVertices(jobId)) {
            result.push_back(std::cref(v));
        }
    }
    return result;
}

cg::VerticesCRef cg::Graph::toConstant(const VerticesRef &vertices) {
    VerticesCRef result;
    for (auto v : vertices) {
        result.push_back(std::cref(v));
    }
    return result;
}

// delayGraph

cg::VerticesCRef cg::ConstraintGraph::getMaintVertices() const {
    VerticesCRef v;
    for (const auto &vert : getVertices()) {
        if (vert.operation.isMaintenance()) {
            v.emplace_back(vert);
        }
    }
    return v;
}

cg::VerticesCRef cg::ConstraintGraph::getSources() const {
    VerticesCRef result;
    const auto &vertices = getVertices();
    std::copy_if(vertices.begin(),
                 vertices.end(),
                 std::back_inserter(result),
                 [&](const Vertex &v) { return isSource(v); });
    return result;
}
