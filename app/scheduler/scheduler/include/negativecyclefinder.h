#ifndef NEGATIVECYCLEFINDER_H
#define NEGATIVECYCLEFINDER_H

#include "delayGraph/delayGraph.h"

#include <optional>
#include <vector>

class NegativeCycleFinder
{
public:
    explicit NegativeCycleFinder(const DelayGraph::delayGraph &graph);

    bool hasNegativeCycle();

    std::vector<DelayGraph::edge> getNegativeCycle();

private:
    void findNegativeCycleDFS(DelayGraph::VertexID v);
    const DelayGraph::delayGraph & graph;
    DelayGraph::Edges negativeCycle;
    std::vector<bool> onStack;
    std::vector<bool> marked;
    std::vector<delay> shortestPath;
    std::vector<std::optional<DelayGraph::edge>> edgeTo;
};

#endif // NEGATIVECYCLEFINDER_H
