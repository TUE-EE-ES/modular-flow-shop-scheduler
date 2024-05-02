#ifndef CYCLEFINDER_H
#define CYCLEFINDER_H

#include "delayGraph/vertex.h"

#include "pch/containers.hpp"

class CycleFinder
{
    std::vector<DelayGraph::VertexID> cycle;
    std::vector<DelayGraph::VertexID> marked;
    bool has_for_cycle_to(unsigned int node_index, std::vector<DelayGraph::VertexID> edgeTo);
    void extract_cycle_to(unsigned int node_index, std::vector<DelayGraph::VertexID> edgeTo);

public:
    explicit CycleFinder(std::vector<DelayGraph::VertexID> edgeTo);
    [[nodiscard]] const std::vector<DelayGraph::VertexID> &getCycle() const { return cycle; }
    [[nodiscard]] bool hasCycle() const { return !cycle.empty(); }
};

#endif // CYCLEFINDER_H
