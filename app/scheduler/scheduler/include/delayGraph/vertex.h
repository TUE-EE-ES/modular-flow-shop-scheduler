#ifndef DELAYGRAPH_VERTEX
#define DELAYGRAPH_VERTEX

#include "FORPFSSPSD/operation.h"
#include "delay.h"
#include "delayGraph/edge.h"
#include "fmsschedulerexception.h"

#include "pch/containers.hpp"

#include <fmt/compile.h>

namespace DelayGraph {

/**
 * @brief A vertex representation
 */
class vertex {
public:
    VertexID id;
    FORPFSSPSD::operation operation;

private:
    FMS::Map<VertexID, decltype(edge::weight)> outgoing_edges;
    FMS::Map<VertexID, decltype(edge::weight)> incoming_edges;

public:
    vertex(VertexID id, FORPFSSPSD::operation operation) : id(id), operation(operation) {}

    vertex(const vertex &other) = default;

    vertex(vertex &&other) = default;

    ~vertex() = default;

    vertex &operator=(vertex &&other) = default;

    vertex &operator=(const vertex &other) = default;

    bool operator==(const vertex &other) const { return id == other.id; }

    [[nodiscard]] vertex copy() const { return {*this}; }

    [[nodiscard]] inline const decltype(outgoing_edges) &get_outgoing_edges() const noexcept {
        return outgoing_edges;
    }
    [[nodiscard]] inline const decltype(incoming_edges) &get_incoming_edges() const noexcept {
        return incoming_edges;
    }

    [[nodiscard]] inline decltype(outgoing_edges) &get_outgoing_edges() noexcept {
        return outgoing_edges;
    }

    [[nodiscard]] edge get_outgoing_edge(VertexID dst) const {
        const auto it = outgoing_edges.find(dst);
        if (it == outgoing_edges.end()) {
            throw FmsSchedulerException(fmt::format(
                    "Unable to retrieve outgoing edge from {} to {}", this->operation, dst));
        }
        return {id, dst, it->second};
    }

    [[nodiscard]] inline edge get_outgoing_edge(const vertex &dst) const {
        return get_outgoing_edge(dst.id);
    }

    [[nodiscard]] inline bool has_outgoing_edge(VertexID dst) const {
        return outgoing_edges.find(dst) != outgoing_edges.end();
    }

    [[nodiscard]] decltype(edge::weight) get_weight(VertexID dst) const {
        try {
            return outgoing_edges.at(dst);
        } catch (const std::out_of_range &e) {
            throw FmsSchedulerException(
                    fmt::format("Unable to retrieve outgoing edge from {} to {}", operation, dst));
        }
    }

    [[nodiscard]] inline decltype(edge::weight) get_weight(const vertex &dst) const {
        return get_weight(dst.id);
    }

    decltype(edge::weight) set_weight(VertexID dst, decltype(edge::weight) weight) {
        outgoing_edges.at(dst) = weight;
        return weight;
    }

    inline decltype(edge::weight) set_weight(const vertex &dst, decltype(edge::weight) weight) {
        return set_weight(dst.id, weight);
    }

    /**
     * @brief Adds an edge from this vertex (source) to another vertex (destination), with the
     * given weight. If the edge already exists, the weight is updated.
     *
     * @param other Destination vertex
     * @param e Data stored in the edge
     */
    inline void add_edge(vertex &other, const edge &e) {
        if (e.src != id) {
            throw FmsSchedulerException(fmt::format(
                    "Attempted to add edge from {} to {} at vertex {}", e.src, e.dst, id));
        }
        add_edge(other, e.weight);
    }

    inline void add_edge(vertex &other, delay weight) {
        outgoing_edges[other.id] = weight;
        other.incoming_edges[id] = weight;
    }

    void remove_edge(vertex &other) { outgoing_edges.erase(other.id); other.incoming_edges.erase(id); }

    [[nodiscard]] inline FORPFSSPSD::operation getOp() const noexcept { return operation; }
};

using Vertices = std::vector<vertex>;

} // namespace DelayGraph

template <> struct fmt::formatter<DelayGraph::vertex> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const DelayGraph::vertex &v, FormatContext &ctx) -> decltype(ctx.out()) {
        return format_to(ctx.out(), FMT_COMPILE("[vertex {}, op={}]"), v.id, v.operation);
    }
};

#endif // DELAYGRAPH_VERTEX
