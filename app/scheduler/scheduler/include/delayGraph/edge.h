#ifndef DELAYGRAPH_EDGE
#define DELAYGRAPH_EDGE

#include "delay.h"

#include <fmt/compile.h>

/*
 * An edge representation
 * */
namespace DelayGraph {

using VertexID = std::size_t;

struct edge {
    VertexID src, dst;
    delay weight;

    constexpr edge(const VertexID &src, const VertexID &dst, delay weight) :
        src(src), dst(dst), weight(weight) {}

    bool operator==(const edge &rhs) const {
        return src == rhs.src && dst == rhs.dst && weight == rhs.weight;
    }
};

using Edges = std::vector<edge>;
} // namespace DelayGraph

template <> struct fmt::formatter<DelayGraph::edge> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const DelayGraph::edge &e, FormatContext &ctx) -> decltype(ctx.out()) {
        return format_to(ctx.out(), FMT_COMPILE("{}-({})->{}"), e.src, e.weight, e.dst);
    }
};

#endif // DELAYGRAPH_EDGE

