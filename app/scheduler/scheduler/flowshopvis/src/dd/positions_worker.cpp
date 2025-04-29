#include "positions_worker.hpp"

#include <fms/dd/vertex.hpp>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <stack>
#include <tuple>
#include <unordered_map>
#include <vector>

using namespace FlowShopVis::DD;
using namespace fms::dd;

void PositionsWorker::run() {
    std::optional<VertexId> maxId = std::nullopt;

    std::unordered_map<std::size_t, std::vector<fms::dd::VertexId>> depthLevels;
    std::optional<fms::dd::VertexId> rootId;
    m_edges.resize(m_data->allStates.size());

    for (const auto &vertex : m_data->allStates) {
        depthLevels[vertex->vertexDepth()].push_back(vertex->id());
        maxId = std::max(maxId.value_or(0), vertex->id());
        if (vertex->id() != vertex->parentId()) {
            m_edges.at(vertex->parentId()).push_back(vertex->id());
        } else {
            rootId = vertex->id();
        }
    }

    if (!rootId.has_value()) {
        emit error("No root node found");
        return;
    }

    if (maxId.value() != m_data->allStates.size() - 1) {
        emit error("Not all nodes are present");
        return;
    }

    auto nodesWidth = computeNodesWidth(*rootId);
    calculatePositions(*rootId, nodesWidth);
    m_rootId = rootId;
    emit positionsCalculated();
}

PositionsWorker::NodesSize PositionsWorker::computeNodesWidth(VertexId rootId) const {
    // Non-recursive post-order traversal of the tree
    std::stack<std::tuple<VertexId, bool>> stack;
    std::unordered_map<VertexId, std::size_t> nodesWidth;
    qlonglong count = 1;

    stack.emplace(rootId, false);
    while (!stack.empty()) {
        auto &[id, visited] = stack.top();

        // const auto itChild = m_edges.find(id);

        const auto &children = m_edges.at(id);
        // if (!visited && itChild != m_edges.end()) {
        if (!visited && !children.empty()) {
            visited = true;
            for (const auto &child : children) {
                stack.emplace(child, false);
            }

            continue;
        }
        stack.pop();
        emit progress(count);
        count++;

        if (children.empty()) {
            // Node is a leaf
            continue;
        }

        // Node has children
        std::size_t width = 0;
        for (const auto &child : children) {
            const auto itChildWidth = nodesWidth.find(child);
            if (itChildWidth == nodesWidth.end()) {
                width += 1;
            } else {
                width += itChildWidth->second;
            }
        }
        nodesWidth.emplace(id, width);
    }

    return nodesWidth;
}

void PositionsWorker::calculatePositions(VertexId rootId, NodesSize nodesWidth) {
    std::stack<std::tuple<VertexId, qreal>> stack;
    stack.emplace(rootId, 0);

    const auto computeWidth = [&nodesWidth, this](VertexId id) {
        const auto itWidth = nodesWidth.find(id);
        const auto width = itWidth == nodesWidth.end() ? 1 : itWidth->second;
        return static_cast<qreal>(width) * m_nodeXSpace;
    };

    while (!stack.empty()) {
        auto &[id, xMin] = stack.top();
        stack.pop();

        const auto node = m_data->allStates.at(id);

        const qreal xWidth = computeWidth(id);
        const qreal xPos = xMin + xWidth / 2.;
        const qreal yPos = static_cast<qreal>(node->vertexDepth()) * m_nodeYSpace;

        m_positions.emplace(id, QPointF(xPos, yPos));
        emit progress(m_data->allStates.size() + static_cast<qlonglong>(m_positions.size()));

        // const auto itChilds = m_edges.find(id);
        const auto children = m_edges.at(id);
        // if (itChilds == m_edges.end()) {
        if (children.empty()) {
            // Node is a leaf
            continue;
        }

        // Node has children
        qreal xOffset = xMin;
        for (const auto &child : children) {
            const auto width = computeWidth(child);
            stack.emplace(child, xOffset);
            xOffset += width;
        }
    }
}
