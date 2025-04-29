/*
 * @author  Joost van Pinxten <j.h.h.v.pinxten@tue.nl>
 * @version 0.2
 */

#ifndef FMS_CG_CONSTRAINT_GRAPH_HPP
#define FMS_CG_CONSTRAINT_GRAPH_HPP

#include "edge.hpp"
#include "vertex.hpp"

#include "fms/delay.hpp"
#include "fms/problem/indices.hpp"
#include "fms/problem/operation.hpp"
#include "fms/scheduler_exception.hpp"
#include "fms/utils/containers.hpp"

#include <fmt/base.h>

/// @brief Namespace for the constraint graph utilities
namespace fms::cg {

using VerticesRef = std::vector<std::reference_wrapper<Vertex>>;
using VerticesCRef = std::vector<std::reference_wrapper<const Vertex>>;
using VerticesIds = std::vector<VertexId>;

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
 * It is also designed to work with FS::operation as an identifier of vertices.
 *
 */
class Graph {

public:
    Graph() = default;
    ~Graph() = default;
    Graph(Graph &&other) noexcept = default;
    Graph(const Graph &other) { *this = other; }

    Graph &operator=(Graph &&other) noexcept = default;
    Graph &operator=(const Graph &other) = default;

    /**
     * @brief Adds a vertex to the graph
     * @param s The operation that the vertex represents
     * @return The ID of the added vertex
     */
    VertexId addVertex(const problem::Operation &s);

    /**
     * @brief Adds a vertex to the graph
     * @param args The arguments to construct an operation
     * @return The ID of the added vertex
     */
    template <typename... Args> VertexId addVertex(Args... args) {
        return addVertex(problem::Operation{args...});
    }

    /**
     * @brief Removes an edge from the graph if it's there.
     * @param e The edge to remove
     */
    // NOLINTNEXTLINE(readability-make-member-function-const)
    inline void removeEdge(const Edge &e) { getVertex(e.src).removeEdge(getVertex(e.dst)); }

    
    /**
     * @brief Overload of @ref remove_edge. Removes an edge from the graph if it's there
     * @details Removes the edge from @p src to @p dst if it exists, otherwise does nothing. 
     * You can use any identifier for @p src and @p dst as long as a @ref Vertex can be 
     * obtained from it.
     * @tparam T1 Type of the source vertex. Can be any that handles @ref getVertex.
     * @tparam T2 Type of the destination vertex. Can be any that handles @ref getVertex.
     * @param src Source vertex
     * @param dst Destination vertex
     */
    template <typename T1, typename T2> inline void removeEdge(T1 &&src, T2 &&dst) {
        getVertex(std::forward<T1>(src)).remove_edge(getVertex(std::forward<T2>(dst)));
    }


    /**
     * @brief Removes multiple edges from the graph
     * @param edges The edges to remove
     */
    void removeEdges(const Edges &edges) {
        for (const auto &e : edges) {
            removeEdge(e);
        }
    }

    /**
     * @brief Adds multiple edges to the graph
     * @param edges The edges to add
     * @return The edges that were added
     */
    Edges addEdges(const Edges &edges);

    /**
     * @brief Adds an edge to the graph
     * @details Adds the edge @p e to the graph and if it already exists, updates the weight.
     * @param e The edge to add
     */
    inline void addEdges(const Edge &e) { getVertex(e.src).addEdge(getVertex(e.dst), e); }

    /**
     * @brief Overload of @ref add_edge
     * @details This overload allows to add the edge from any type that can be used to obtain an
     * existing @ref Vertex from the graph using @ref getVertex. The edge is added from @p from to
     * @p to with the given @p weight . If an edge already exists, the weight is updated.
     * @tparam T1 Type of the source vertex. Can be any that handles @ref getVertex.
     * @tparam T2 Type of the destination vertex. Can be any that handles @ref getVertex.
     * @param from Source vertex.
     * @param to Destination vertex.
     * @param weight Weight of the edge.
     * @return Edge Newly added edge.
     */
    template <typename T1, typename T2> Edge addEdge(T1 &&from, T2 &&to, delay weight) {
        auto &fromV = getVertex(std::forward<T1>(from));
        auto &toV = getVertex(std::forward<T2>(to));

        fromV.addEdge(toV, weight);
        return {fromV.id, toV.id, weight};
    }

    /**
     * @brief Adds or updates an edge in the graph
     * @details Adds an edge from @p from to @p to with the given @p weight if it does not exist,
     * otherwise updates the existing weight to @p weight .
     * @return The edge that was added or updated
     */
    template <typename T1, typename T2> Edge addOrUpdateEdge(T1 &&from, T2 &&to, delay weight) {
        auto &fromV = getVertex(std::forward<T1>(from));
        auto &toV = getVertex(std::forward<T2>(to));

        if (fromV.has_outgoing_edge(toV.id)) {
            Edge e = fromV.get_outgoing_edge(toV.id);
            e.weight = weight;
            return e;
        }
        Edge e(fromV.id, toV.id, weight);
        fromV.add_edge(toV, e);
        return e;
    }

    [[nodiscard]] inline size_t getNumberOfVertices() const noexcept { return vertices.size(); }

    [[nodiscard]] inline const Vertex &getVertex(const VertexId &vertexId) const {
        if (vertexId >= getNumberOfVertices()) {
            throw FmsSchedulerException(fmt::format("Vertex ID {0} out of range! 0 <= {0} < {1}",
                                                    vertexId,
                                                    getNumberOfVertices()));
        }
        return vertices.at(vertexId);
    }

    [[nodiscard]] const Vertex &getVertex(const problem::Operation &op) const {
        auto it = identifierToVertex.find(op);
        if (it == identifierToVertex.end()) {
            throw FmsSchedulerException(fmt::format(
                    "Error, unable to find the vertex for the given operation ({}) in the graph",
                    op));
        }
        return vertices.at(it->second);
    }

    [[nodiscard]] static inline const Vertex &getVertex(const Vertex &v) { return v; }

    template <typename T> [[nodiscard]] inline Vertex &getVertex(T &&v) {
        // From: https://stackoverflow.com/a/856839/4005637
        // NOLINTBEGIN: This allows us to reimplement the method for const and non-const
        return const_cast<Vertex &>(
                const_cast<const Graph *>(this)->getVertex(std::forward<T>(v)));
        // NOLINTEND
    }

    [[nodiscard]] inline Vertex &getVertex(const problem::Operation &op) {
        // NOLINTNEXTLINE
        return const_cast<Vertex &>(const_cast<const Graph *>(this)->getVertex(op));
    }

    template <typename T> [[nodiscard]] inline VertexId getVertexId(const T &v) const {
        return getVertex(v).id;
    }

    [[nodiscard]] inline VertexId getVertexId(const problem::Operation &op) const {
        auto it = identifierToVertex.find(op);
        if (it == identifierToVertex.end()) {
            throw FmsSchedulerException(fmt::format(
                    "Error, unable to find the vertex for the given operation ({}) in the graph",
                    op));
        }
        return it->second;
    }

    [[nodiscard]] static inline VertexId getVertexId(const Vertex &v) { return v.id; }

    [[nodiscard]] static inline VertexId getVertexId(VertexId id) { return id; }

    /// @brief Gets the operation associated with a vertex
    /// @details It is possible that the operation is not valid (e.g., for a source or terminus)
    /// so, it is recommended to check if the operation is valid before using it by using
    /// @ref problem::Operation::isValid
    [[nodiscard]] inline const problem::Operation &
    getOperation(const VertexId &vertexId) const {
        const auto &v = getVertex(vertexId);
        return v.operation;
    }

    [[nodiscard]] inline bool hasVertex(const problem::Operation &op) const {
        return identifierToVertex.find(op) != identifierToVertex.end();
    }

    template <typename T,
              typename std::enable_if_t<!std::is_same_v<T, problem::Operation &>, int> = 0>
    [[nodiscard]] inline bool hasVertex(T &&v) const noexcept {
        return getVertexId(std::forward<T>(v)) < getNumberOfVertices();
    }

    template <typename T1, typename T2>
    [[nodiscard]] bool hasEdge(const T1 &src, const T2 &dst) const {
        return getVertex(src).hasOutgoingEdge(getVertexId(dst));
    }

    [[nodiscard]] bool hasEdge(const Edge &e) const { return hasEdge(e.src, e.dst); }

    /**
     * @brief Retrieve the edge between two vertices
     *
     * @param src Identifier of the source vertex
     * @param dst Identifier of the destination vertex
     * @return const edge&
     */
    template <typename T1, typename T2>
    [[nodiscard]] Edge getEdge(const T1 &src, const T2 &dst) const {
        return getVertex(src).getOutgoingEdge(getVertexId(dst));
    }

    template <typename T1, typename T2>
    [[nodiscard]] decltype(Edge::weight) getWeight(const T1 &src, const T2 &dst) const {
        return getVertex(src).get_weight(getVertexId(dst));
    }

    [[nodiscard]] inline const Vertices &getVertices() const { return vertices; }

    [[nodiscard]] inline Vertices &getVertices() { return vertices; }

    [[nodiscard]] VerticesCRef getVertices(problem::JobId jobId) const;

    [[nodiscard]] VerticesRef getVertices(problem::JobId jobId);

    [[nodiscard]] VerticesCRef getVertices(const std::vector<problem::JobId> &jobIds) const;

    [[nodiscard]] VerticesCRef getVertices(problem::JobId start_id,
                                            problem::JobId end_id) const;

    template <typename... T> [[nodiscard]] inline VerticesCRef getVerticesC(T &&...job) const {
        return getVertices(std::forward<T>(job)...);
    }

    [[nodiscard]] VerticesCRef getVerticesC() const;

    /**
     * @brief Converts a list of vertices to a constant list of vertices
     * @param vertices The list of vertices
     * @return The constant list of vertices
     */
    [[nodiscard]] static VerticesCRef toConstant(const VerticesRef &vertices);

private:
    VertexId lastId{};

    /// List of vertices, the VertexId is implicitly the index of the vector
    std::vector<Vertex> vertices;

    /// Maps the custom identifier to its respective vertex
    utils::containers::Map<problem::Operation, VertexId> identifierToVertex;

    utils::containers::Map<problem::JobId, std::vector<VertexId>> jobToVertex;
};

class ConstraintGraph : public Graph {
public:
    // JobId::max() is reserved for invalid operations
    constexpr const static problem::JobId SOURCE_ID{problem::JobId::max() - 1U};
    constexpr const static problem::JobId TERMINAL_ID{problem::JobId::max() - 2U};
    constexpr const static problem::JobId NEXT_ID{problem::JobId::max() - 3U};

    /// @brief Constant for terminal operation
    constexpr const static problem::Operation OP_TERMINAL{TERMINAL_ID, 0};

    /**
     * @brief Adds a machine source vertex to the graph
     * @param sourceId The machine ID for the source
     * @return The ID of the added vertex
     */
    VertexId addSource(problem::MachineId sourceId) {
        return addVertex(SOURCE_ID, static_cast<problem::OperationId>(sourceId));
    }

    /**
     * @brief Adds a terminus vertex to the graph
     * @return The ID of the added vertex
     */
    VertexId addTerminus() { return addVertex(OP_TERMINAL); }

    /**
     * @brief Gets the machine ID for a source vertex
     * @param v The vertex to get the machine ID from
     * @return The machine ID of the source vertex
     */
    template <typename T> [[nodiscard]] problem::MachineId getSourceMachine(T &&v) const {
        const auto &vRef = getVertex(std::forward<T>(v));

        if (!isSource(vRef)) {
            throw FmsSchedulerException(
                    fmt::format("Error, the given vertex ({}) is not a source vertex", vRef.id));
        }
        return static_cast<problem::MachineId>(vRef.operation.operationId);
    }

    [[nodiscard]] inline static constexpr bool isSource(const Vertex &v) {
        return v.operation.jobId == SOURCE_ID;
    }

    template <typename T> [[nodiscard]] inline bool isSource(T &&v) const {
        return isSource(getVertex(std::forward<T>(v)));
    }

    [[nodiscard]] inline static constexpr bool isTerminus(const Vertex &v) {
        return v.operation.jobId == TERMINAL_ID;
    }

    template <typename T> [[nodiscard]] inline bool isTerminus(T &&v) const {
        return isTerminus(getVertex(std::forward<T>(v)));
    }

    [[nodiscard]] inline static constexpr bool isVisible(const Vertex &v) {
        auto jobId = v.operation.jobId;
        return jobId != SOURCE_ID && !v.operation.isMaintenance() && jobId != TERMINAL_ID;
    }

    template <typename T> [[nodiscard]] inline bool isVisible(T &&v) const {
        return isVisible(getVertex(std::forward<T>(v)));
    }

    [[nodiscard]] VerticesCRef getSources() const;

    [[nodiscard]] VerticesCRef getMaintVertices() const;

    [[nodiscard]] inline const Vertex &getSource(problem::MachineId machineId) const {
        return getVertex({SOURCE_ID, static_cast<problem::OperationId>(machineId)});
    }

    template <typename T> [[nodiscard]] inline Vertex &getSource(T &&v) {
        // NOLINTBEGIN: This allows us to reimplement the method for const and non-const
        return const_cast<Vertex &>(
                const_cast<const ConstraintGraph *>(this)->getSource(std::forward<T>(v)));
        // NOLINTEND
    }

    [[nodiscard]] inline const Vertex &getTerminus() const { return getVertex(OP_TERMINAL); }

    [[nodiscard]] inline Vertex &getTerminus() { return getVertex(OP_TERMINAL); }
};
} // namespace fms::cg

#endif /* FMS_CG_CONSTRAINT_GRAPH_HPP */
