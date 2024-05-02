#include "DD/vertex.hpp"
#include "delayGraph/edge.h"

using namespace DD;

DelayGraph::Edges Vertex::getAllEdges() const {
    DelayGraph::Edges allEdges;

    // At least we'll need as space as number of scheduling decisions
    allEdges.reserve(m_vertexDepth);
    for (const auto &[machine, edges] : m_machineEdges) {
        allEdges.insert(allEdges.end(), edges.begin(), edges.end());
    }
    return allEdges;
}
