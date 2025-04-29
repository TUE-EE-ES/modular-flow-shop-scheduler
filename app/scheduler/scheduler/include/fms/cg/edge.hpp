#ifndef FMS_CG_EDGE_HPP
#define FMS_CG_EDGE_HPP

#include "fms/delay.hpp"

#include <fmt/compile.h>

/*
 * An edge representation
 * */
namespace fms::cg {

using VertexId = std::size_t;

/**
 * @brief A struct representing an edge in a graph
 */
struct Edge {
    /// The source and destination vertices of the edge
    VertexId src, dst; 

    /// The weight of the edge
    delay weight; 

    /**
     * @brief Constructs an edge
     * @param src The source vertex of the edge
     * @param dst The destination vertex of the edge
     * @param weight The weight of the edge
     */
    constexpr Edge(const VertexId &src, const VertexId &dst, delay weight) :
        src(src), dst(dst), weight(weight) {}

    /// @brief Compares two edges for equality
    /// @details Two edges are equal if their source, destination, and weight are equal
    /// @return True if the edges are equal, false otherwise
    bool operator==(const Edge &rhs) const {
        return src == rhs.src && dst == rhs.dst && weight == rhs.weight;
    }
};

/// A type alias for a vector of edges
using Edges = std::vector<Edge>;
}  // namespace fms::cg

template <> struct fmt::formatter<fms::cg::Edge> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const fms::cg::Edge &e, FormatContext &ctx) const -> decltype(ctx.out()) {
        return format_to(ctx.out(), FMT_COMPILE("{}-({})->{}"), e.src, e.weight, e.dst);
    }
};

#endif // FMS_CG_EDGE_HPP
