#ifndef FMS_CG_VERTEX_HPP
#define FMS_CG_VERTEX_HPP

#include "edge.hpp"

#include "fms/delay.hpp"
#include "fms/problem/operation.hpp"
#include "fms/scheduler_exception.hpp"
#include "fms/utils/containers.hpp"

#include <fmt/compile.h>

namespace fms::cg {

/**
 * @brief A vertex representation
 */
class Vertex {
public:
    VertexId id;
    problem::Operation operation;

private:
    utils::containers::Map<VertexId, decltype(Edge::weight)> outgoing_edges;
    utils::containers::Map<VertexId, decltype(Edge::weight)> incoming_edges;

public:
    /**
     * @brief Constructs a vertex
     * @param id The ID of the vertex
     * @param operation The operation associated with the vertex
     */
    Vertex(VertexId id, problem::Operation operation) : id(id), operation(operation) {}

    Vertex(const Vertex &other) = default;

    Vertex(Vertex &&other) = default;

    ~Vertex() = default;

    Vertex &operator=(Vertex &&other) = default;

    Vertex &operator=(const Vertex &other) = default;

    /**
     * @brief Compares two vertices for equality
     * @details Two vertices are equal if their IDs are equal
     * @return True if the vertices are equal, false otherwise
     */
    bool operator==(const Vertex &other) const { return id == other.id; }

    /**
     * @brief Gets the incoming edges to the vertex
     * @return The incoming edges to the vertex
     */
    [[nodiscard]] inline const decltype(incoming_edges) &getIncomingEdges() const noexcept {
        return incoming_edges;
    }

    /**
     * @brief Gets the outgoing edges from the vertex
     * @return The outgoing edges from the vertex
     */
    [[nodiscard]] inline const decltype(outgoing_edges) &getOutgoingEdges() const noexcept {
        return outgoing_edges;
    }

    /**
     * @brief Gets the outgoing edges from the vertex
     * @return The outgoing edges from the vertex
     */
    [[nodiscard]] inline decltype(outgoing_edges) &getOutgoingEdges() noexcept {
        return outgoing_edges;
    }

    /**
     * @brief Retrieves the outgoing edge from this vertex to the specified destination vertex.
     * @param dst The ID of the destination vertex.
     * @return The edge from this vertex to the destination vertex.
     * @throws FmsSchedulerException if there is no outgoing edge to the destination vertex.
     */
    [[nodiscard]] Edge getOutgoingEdge(VertexId dst) const {
        const auto it = outgoing_edges.find(dst);
        if (it == outgoing_edges.end()) {
            throw FmsSchedulerException(fmt::format(
                    "Unable to retrieve outgoing edge from {} to {}", this->operation, dst));
        }
        return {id, dst, it->second};
    }

    /**
     * @brief Retrieves the outgoing edge from this vertex to the specified destination vertex.
     * @param dst The destination vertex.
     * @return The edge from this vertex to the destination vertex.
     */
    [[nodiscard]] inline Edge getOutgoingEdge(const Vertex &dst) const {
        return getOutgoingEdge(dst.id);
    }

    /**
     * @brief Checks if there is an outgoing edge from this vertex to the specified destination
     * vertex.
     * @param dst The ID of the destination vertex.
     * @return True if there is an outgoing edge to the destination vertex, false otherwise.
     */
    [[nodiscard]] inline bool hasOutgoingEdge(VertexId dst) const {
        return outgoing_edges.find(dst) != outgoing_edges.end();
    }

    /**
     * @brief Retrieves the weight of the outgoing edge from this vertex to the specified
     * destination vertex.
     * @param dst The ID of the destination vertex.
     * @return The weight of the outgoing edge to the destination vertex.
     * @throws FmsSchedulerException if there is no outgoing edge to the destination vertex.
     */
    [[nodiscard]] decltype(Edge::weight) getWeight(VertexId dst) const {
        try {
            return outgoing_edges.at(dst);
        } catch (const std::out_of_range &e) {
            throw FmsSchedulerException(
                    fmt::format("Unable to retrieve outgoing edge from {} to {}", operation, dst));
        }
    }

    /**
     * @brief Retrieves the weight of the outgoing edge from this vertex to the specified
     * destination vertex.
     * @param dst The destination vertex.
     * @return The weight of the outgoing edge to the destination vertex.
     * @throws FmsSchedulerException if there is no outgoing edge to the destination vertex.
     */
    [[nodiscard]] inline decltype(Edge::weight) getWeight(const Vertex &dst) const {
        return getWeight(dst.id);
    }

    /**
     * @brief Sets the weight of the outgoing edge.
     * @details Sets the weight of the outgoing edge @p dst to @p weight .
     * @return The new weight of the outgoing edge.
     * @throws std::out_of_range if there is no outgoing edge to the destination vertex.
     */
    decltype(Edge::weight) setWeight(VertexId dst, decltype(Edge::weight) weight) {
        outgoing_edges.at(dst) = weight;
        return weight;
    }

    inline decltype(Edge::weight) setWeight(const Vertex &dst, decltype(Edge::weight) weight) {
        return setWeight(dst.id, weight);
    }

    /**
     * @brief Adds an edge from this vertex (source) to another vertex (destination), with the
     * given weight. If the edge already exists, the weight is updated.
     *
     * @param other Destination vertex
     * @param e Data stored in the edge
     */
    inline void addEdge(Vertex &other, const Edge &e) {
        if (e.src != id) {
            throw FmsSchedulerException(fmt::format(
                    "Attempted to add edge from {} to {} at vertex {}", e.src, e.dst, id));
        }
        addEdge(other, e.weight);
    }

    /**
     * @brief Adds an edge from this vertex (source) to another vertex (destination), with the
     * @param other  Destination vertex
     * @param weight  Weight of the edge
     */
    inline void addEdge(Vertex &other, delay weight) {
        outgoing_edges[other.id] = weight;
        other.incoming_edges[id] = weight;
    }

    /**
     * @brief Removes the edge from this vertex to the specified destination vertex.
     * @param other  Destination vertex
     */
    inline void removeEdge(Vertex &other) {
        outgoing_edges.erase(other.id);
        other.incoming_edges.erase(id);
    }

    [[nodiscard]] inline problem::Operation getOp() const noexcept { return operation; }
};

using Vertices = std::vector<Vertex>;

} // namespace fms::cg

template <> struct fmt::formatter<fms::cg::Vertex> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const fms::cg::Vertex &v, FormatContext &ctx) const -> decltype(ctx.out()) {
        return format_to(ctx.out(), FMT_COMPILE("[vertex {}, op={}]"), v.id, v.operation);
    }
};

#endif // FMS_CG_VERTEX_HPP
