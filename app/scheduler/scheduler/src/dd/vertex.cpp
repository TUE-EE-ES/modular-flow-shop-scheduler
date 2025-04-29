#include "fms/pch/containers.hpp"

#include "fms/dd/vertex.hpp"

#include "fms/cg/edge.hpp"

using namespace fms;
using namespace fms::dd;

cg::Edges Vertex::getAllEdges(const problem::Instance &problem) const {
    const auto &g = problem.getDelayGraph();
    cg::Edges allEdges;

    // At least we'll need as space as number of scheduling decisions
    allEdges.reserve(m_vertexDepth);
    for (const auto &[machine, ops] : m_machinesSequences) {
        std::reference_wrapper<const cg::Vertex> prevV = g.getSource(machine);

        for (const auto &op : ops) {
            const auto &v = g.getVertex(op);
            allEdges.emplace_back(prevV.get().id, v.id, problem.query(prevV, v));
            prevV = v;
        }
    }
    return allEdges;
}
