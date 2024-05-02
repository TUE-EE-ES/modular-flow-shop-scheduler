/*
 * @author  Joost van Pinxten <j.h.h.v.pinxten@tue.nl>
 * @version 0.2
 */

#ifndef DELAY_GRAPH_H_
#define DELAY_GRAPH_H_

#include "FORPFSSPSD/indices.hpp"
#include "FORPFSSPSD/operation.h"
#include "delay.h"
#include "delayGraph/edge.h"
#include "delayGraph/vertex.h"
#include "fmsschedulerexception.h"

#include "pch/containers.hpp"

#include <algorithm>
#include <fmt/core.h>

class PartialSolution;
namespace DelayGraph {

using VerticesRef = std::vector<std::reference_wrapper<vertex>>;
using VerticesCRef = std::vector<std::reference_wrapper<const vertex>>;
using VerticesIDs = std::vector<VertexID>;

/*
 * An adjacency list implementation of a graph; particularly efficient
 * for sparse graphs.
 *
 * This particular implementation does _not_ allow removal of vertices,
 * but it does allow arbitrary adding and removing edges.
 *
 * It is also not a multi-graph, a directed edge may occur between two
 * nodes at most once. I.e. no parallel directed edges are allowed.
 *
 * It is also designed to work with FORPFSSPSD::operation as an identifier of vertices.
 *
 */
class graph {

public:
    graph() = default;
    ~graph() = default;
    graph(graph &&other) noexcept = default;
    graph(const graph &other) { *this = other; }

    graph &operator=(graph &&other) noexcept = default;
    graph &operator=(const graph &other) = default;
    
    VertexID add_vertex(const FORPFSSPSD::operation &s) {
        VertexID id = lastId++;

        auto &v = vertices.emplace_back(id, s);
        identifierToVertex[s] = id;
        jobToVertex[s.jobId].emplace_back(v.id);
        return id;
    }

    template <typename... Args> VertexID add_vertex(Args... args) {
        return add_vertex(FORPFSSPSD::operation{args...});
    }

    // NOLINTNEXTLINE(readability-make-member-function-const)
    inline void remove_edge(const edge &e) { get_vertex(e.src).remove_edge(get_vertex(e.dst)); }

    void remove_edges(const Edges &edges) {
        for (const auto &e : edges) {
            remove_edge(e);
        }
    }

    template <typename T1, typename T2> inline void remove_edge(T1 &&src, T2 &&dst) {
        get_vertex(std::forward<T1>(src)).remove_edge(get_vertex(std::forward<T2>(dst)));
    }

    Edges add_edges(const Edges &edges) {
        Edges addedEdges;
        addedEdges.reserve(edges.size());
        for (const auto &e : edges) {
            if (!has_edge(e.src, e.dst)) {
                add_edge(e);
                addedEdges.emplace_back(e);
                // LOG("Actually added to dg egde from {} to {}",e.src,e.dst);
            }
            // else{
            //     LOG("Edge from {} to {} already exists with weight {}",e.src,e.dst,get_edge(e.src,e.dst).weight);
            // }
        }
        return addedEdges;
    }

    inline void add_edge(const edge &e) { get_vertex(e.src).add_edge(get_vertex(e.dst), e); }

    template <typename T1, typename T2> edge add_edge(T1 &&from, T2 &&to, delay weight) {
        auto &fromV = get_vertex(std::forward<T1>(from));
        auto &toV = get_vertex(std::forward<T2>(to));

        fromV.add_edge(toV, weight);
        return {fromV.id, toV.id, weight};
    }

    template <typename T1, typename T2> edge add_or_update_edge(T1 &&from, T2 &&to, delay weight) {
        auto &fromV = get_vertex(std::forward<T1>(from));
        auto &toV = get_vertex(std::forward<T2>(to));

        if (fromV.has_outgoing_edge(toV.id)) {
            edge e = fromV.get_outgoing_edge(toV.id);
            e.weight = weight;
            return e;
        }
        edge e(fromV.id, toV.id, weight);
        fromV.add_edge(toV, e);
        return e;
    }

    [[nodiscard]] inline size_t get_number_of_vertices() const noexcept { return vertices.size(); }

    [[nodiscard]] inline const vertex &get_vertex(const VertexID &vertexId) const {
        if (vertexId >= get_number_of_vertices()) {
            throw FmsSchedulerException(fmt::format("Vertex ID {0} out of range! 0 <= {0} < {1}",
                                                    vertexId,
                                                    get_number_of_vertices()));
        }
        return vertices.at(vertexId);
    }

    [[nodiscard]] const vertex &get_vertex(const FORPFSSPSD::operation &op) const {
        auto it = identifierToVertex.find(op);
        if (it == identifierToVertex.end()) {
            throw FmsSchedulerException(fmt::format(
                    "Error, unable to find the vertex for the given operation ({}) in the graph",
                    op));
        }
        return vertices.at(it->second);
    }

    // TO DO: Find a better way to retrieve the operation
    [[nodiscard]] const FORPFSSPSD::operation &get_operation(const VertexID &vertexId) const {
        for (const auto& [key, value] : identifierToVertex){
            if (value == vertexId) {
                return key;
            }
        }
        throw FmsSchedulerException(fmt::format(
                    "Error, unable to find the operation for the given vertex ({}) in the graph",
                    vertexId));
    }

    [[nodiscard]] static inline const vertex &
    get_vertex(const vertex &v) {
        return v;
    }

    template <typename T> [[nodiscard]] inline vertex &get_vertex(T &&v) {
        // From: https://stackoverflow.com/a/856839/4005637
        // NOLINTBEGIN: This allows us to reimplement the method for const and non-const
        return const_cast<vertex &>(
                const_cast<const graph *>(this)->get_vertex(std::forward<T>(v)));
        // NOLINTEND
    }

    [[nodiscard]] inline vertex& get_vertex(const FORPFSSPSD::operation &op) {
        // NOLINTNEXTLINE
        return const_cast<vertex &>(const_cast<const graph *>(this)->get_vertex(op));
    }

    template <typename T> [[nodiscard]] inline VertexID get_vertex_id(const T &v) const {
        return get_vertex(v).id;
    }

    [[nodiscard]] inline VertexID get_vertex_id(const FORPFSSPSD::operation& op) const {
        auto it = identifierToVertex.find(op);
        if (it == identifierToVertex.end()) {
            throw FmsSchedulerException(fmt::format(
                    "Error, unable to find the vertex for the given operation ({}) in the graph",
                    op));
        }
        return it->second;
    }

    [[nodiscard]] static inline VertexID get_vertex_id(const vertex &v) { return v.id; }

    [[nodiscard]] static inline VertexID get_vertex_id(VertexID id) { return id; }

    [[nodiscard]] inline bool has_vertex(const FORPFSSPSD::operation &op) const {
        return identifierToVertex.find(op) != identifierToVertex.end();
    }

    template <typename T,
              typename std::enable_if_t<!std::is_same_v<T, FORPFSSPSD::operation &>, int> = 0>
    [[nodiscard]] inline bool has_vertex(T &&v) const noexcept {
        return get_vertex_id(std::forward<T>(v)) < get_number_of_vertices();
    }

    template <typename T1, typename T2>
    [[nodiscard]] bool has_edge(const T1 &src, const T2 &dst) const {
        return get_vertex(src).has_outgoing_edge(get_vertex_id(dst));
    }

    [[nodiscard]] bool has_edge(const edge &e) const { return has_edge(e.src, e.dst); }

    /**
     * @brief Retrieve the edge between two vertices
     *
     * @param src Identifier of the source vertex
     * @param dst Identifier of the destination vertex
     * @return const edge&
     */
    template <typename T1, typename T2>
    [[nodiscard]] edge get_edge(const T1 &src, const T2 &dst) const {
        return get_vertex(src).get_outgoing_edge(get_vertex_id(dst));
    }

    template <typename T1, typename T2>
    [[nodiscard]] decltype(edge::weight) get_weight(const T1 &src, const T2 &dst) const {
        return get_vertex(src).get_weight(get_vertex_id(dst));
    }

    [[nodiscard]] inline const Vertices &get_vertices() const {
        return vertices;
    }

    [[nodiscard]] inline Vertices &get_vertices() {
        return vertices;
    }

    [[nodiscard]] VerticesCRef get_vertices(FORPFSSPSD::JobId jobId) const {
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

    [[nodiscard]] VerticesRef get_vertices(FORPFSSPSD::JobId jobId) {
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

    [[nodiscard]] VerticesCRef
    get_vertices(const std::vector<FORPFSSPSD::JobId> &jobIds) const {
        VerticesCRef result;
        for (auto jobId : jobIds) {
            for (const auto &v : get_vertices(jobId)) {
                result.push_back(std::cref(v));
            }
        }
        return result;
    }

    [[nodiscard]] VerticesCRef
    get_vertices(FORPFSSPSD::JobId start_id, FORPFSSPSD::JobId end_id) const {
        VerticesCRef result;
        for (auto id = start_id; id <= end_id; ++id) {
            for (const auto &v: get_vertices(id)) {
                result.push_back(std::cref(v));
            }
        }
        return result;
    }

    template <typename... T> [[nodiscard]] inline VerticesCRef cget_vertices(T &&...job) const {
        return get_vertices(std::forward<T>(job)...);
    }

    [[nodiscard]] VerticesCRef cget_vertices() const {
        VerticesCRef result;
        for (const auto &v : vertices) {
            result.push_back(std::cref(v));
        }
        return result;
    }

    [[nodiscard]] static VerticesCRef to_constant(const VerticesRef &vertices) {
        VerticesCRef result;
        for (auto v : vertices) {
            result.push_back(std::cref(v));
        }
        return result;
    }

private:
    VertexID lastId{};

    /// List of vertices, the VertexID is implicitly the index of the vector
    std::vector<vertex> vertices;

    /// Maps the custom identifier to its respective vertex
    FMS::Map<FORPFSSPSD::operation, VertexID> identifierToVertex;

    FMS::Map<FORPFSSPSD::JobId, std::vector<VertexID>> jobToVertex;
};

class delayGraph : public graph {
public:
    const static FORPFSSPSD::JobId SOURCE_ID = std::numeric_limits<FORPFSSPSD::JobId>::max();
    const static FORPFSSPSD::JobId TERMINAL_ID = std::numeric_limits<FORPFSSPSD::JobId>::max() - 1;
    const static FORPFSSPSD::JobId MAINT_ID = std::numeric_limits<FORPFSSPSD::JobId>::max() - 2;

    constexpr static FORPFSSPSD::operation OP_TERMINAL = {TERMINAL_ID, 0};

    VertexID add_source(FORPFSSPSD::MachineId sourceId) {
        return add_vertex(SOURCE_ID, static_cast<FORPFSSPSD::OperationId>(sourceId));
    }

    VertexID add_terminus() {
        return add_vertex(OP_TERMINAL);
    }

    VertexID add_maint(FORPFSSPSD::OperationId operationId, uint32_t actionID) {
        return add_vertex(MAINT_ID, operationId, actionID);
    }

    template <typename T> [[nodiscard]] FORPFSSPSD::MachineId get_source_machine(T &&v) const {
        const auto &vRef = get_vertex(std::forward<T>(v));

        if (!is_source(vRef)) {
            throw FmsSchedulerException(fmt::format(
                    "Error, the given vertex ({}) is not a source vertex", vRef.id));
        }
        return static_cast<FORPFSSPSD::MachineId>(vRef.operation.operationId);
    }

    [[nodiscard]] inline static constexpr bool is_source(const vertex &v) {
        return v.operation.jobId == SOURCE_ID;
    }

    template <typename T> [[nodiscard]] inline bool is_source(T &&v) const {
        return is_source(get_vertex(std::forward<T>(v)));
    }

    [[nodiscard]] inline static constexpr bool is_terminus(const vertex &v) {
        return v.operation.jobId == TERMINAL_ID;
    }

    template <typename T> [[nodiscard]] inline bool is_terminus(T &&v) const {
        return is_terminus(get_vertex(std::forward<T>(v)));
    }

    [[nodiscard]] inline static constexpr bool is_maint(const vertex &v) {
        return v.operation.jobId == MAINT_ID;
    }

    template<typename T> [[nodiscard]] inline bool is_maint(T &&v) const {
        return is_maint(get_vertex(std::forward<T>(v)));
    }

    [[nodiscard]] inline static constexpr bool is_visible(const vertex &v) {
        auto jobId = v.operation.jobId;
        return jobId != SOURCE_ID && jobId != MAINT_ID && jobId != TERMINAL_ID;
    }

    template<typename T> [[nodiscard]] inline bool is_visible(T &&v) const {
        return is_visible(get_vertex(std::forward<T>(v)));
    }

    [[nodiscard]] VerticesCRef get_sources() const {
        VerticesCRef result;
        const auto &vertices = get_vertices();
        std::copy_if(vertices.begin(),
                     vertices.end(),
                     std::back_inserter(result),
                     [&](const vertex &v) { return is_source(v); });
        return result;
    }

    [[nodiscard]] VerticesCRef get_maint_vertices() const {
        VerticesCRef v;
        for(const auto& vert : get_vertices()) {
            if (is_maint(vert)){
                v.emplace_back(vert);
            }
        }
        return v;
    }

    [[nodiscard]] inline const vertex &get_source(FORPFSSPSD::MachineId machineId) const {
        return get_vertex({SOURCE_ID, static_cast<FORPFSSPSD::OperationId>(machineId)});
    }

    [[nodiscard]] inline const vertex &get_terminus() const {
        return get_vertex(OP_TERMINAL);
    }

    [[nodiscard]] inline vertex &get_terminus() {
        return get_vertex(OP_TERMINAL);
    }

    template <typename T> [[nodiscard]] inline vertex &get_source(T &&v) {
        // NOLINTBEGIN: This allows us to reimplement the method for const and non-const
        return const_cast<vertex &>(
                const_cast<const delayGraph *>(this)->get_source(std::forward<T>(v)));
        // NOLINTEND
    }
};
} // namespace DelayGraph

#endif /* DELAY_GRAPH_H_ */
