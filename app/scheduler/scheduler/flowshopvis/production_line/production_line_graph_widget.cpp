#include "production_line_graph_widget.hpp"

#include "graph/edge.hpp"
#include "graph/node.hpp"

using namespace FlowShopVis;
using namespace DelayGraph;

namespace {
Edge *findEdge(Node *nodeFrom, Node *nodeTo) {
    for (auto *const edge : nodeFrom->edges()) {
        if (edge->destNode() == nodeTo) {
            return edge;
        }
    }
    return nullptr;
}
} // namespace

static constexpr std::int32_t kBBoxMargin = 20;

ProductionLineGraphWidget::ProductionLineGraphWidget(
        const FORPFSSPSD::ProductionLine &productionLine, QWidget *parent) :
    BasicGraphWidget(parent) {

    // Assume that all modules have their graph initialized

    const auto &moduleIds = productionLine.moduleIds();

    QRectF boundingBox;
    QPointF offset(0, 0);

    std::size_t machineIndex = 0;
    for (const auto moduleId : moduleIds) {
        const auto &module = productionLine.getModule(moduleId);
        std::unordered_map<FORPFSSPSD::MachineId, std::size_t> machineToIndex;
        for (const auto &machineId : module.getMachines()) {
            machineToIndex[machineId] = machineIndex++;
        }

        const auto &dg = module.getDelayGraph();

        // Add all vertices
        double maxY = 0;
        for (const vertex &v : dg.get_vertices()) {
            if (!delayGraph::is_visible(v)) {
                continue;
            }
            const auto &op = v.operation;
            QPointF pos(100 * op.jobId.value, 200 * op.operationId);
            pos += offset;
            maxY = std::max(maxY, pos.y());

            QColor color(getColor(machineToIndex.at(module.getMachine(op))));

            addNode(moduleId, op, pos, color, boundingBox);
        }
        offset = QPointF(0, maxY + 200);

        addModuleEdges(moduleId, dg, boundingBox);
    }

    // Add transfer points
    QPen penSetup(Qt::GlobalColor::darkYellow);
    QPen penDueDate(Qt::GlobalColor::magenta);

    for (const auto &[moduleIdFrom, modulesTo] : productionLine.getTransferConstraints()) {
        const auto &nodesFrom = getNodes().at(moduleIdFrom);
        const auto &moduleFrom = productionLine.getModule(moduleIdFrom);
        for (const auto &[moduleIdTo, point] : modulesTo) {
            const auto &nodesTo = getNodes().at(moduleIdTo);
            const auto &moduleTo = productionLine.getModule(moduleIdTo);

            for (const auto &[jobId, ops] : moduleFrom.jobs()) {
                const auto &opFrom = ops.back();
                const auto &opTo = moduleTo.jobs(jobId).front();

                auto *const nodeFrom = nodesFrom.at(opFrom);
                auto *const nodeTo = nodesTo.at(opTo);

                delay time = moduleFrom.getProcessingTime(opFrom) + point.setupTime(jobId);
                addEdge(nodeFrom, nodeTo, time, 0, penSetup, boundingBox);
            }

            for (const auto &[jobId, time] : point.dueDate) {
                const auto &opFrom = moduleFrom.jobs(jobId).back();
                const auto &opTo = moduleTo.jobs(jobId).front();

                auto *const nodeFrom = nodesFrom.at(opFrom);
                auto *const nodeTo = nodesTo.at(opTo);

                // Due date edges are inverted
                addEdge(nodeTo, nodeFrom, -time, 30, penDueDate, boundingBox);
            }
        }
    }

    setSceneRect(boundingBox.adjusted(-kBBoxMargin, -kBBoxMargin, kBBoxMargin, kBBoxMargin));
}

void ProductionLineGraphWidget::setSequencesHistory(
        const FORPFSSPSD::ProductionLine &productionLine,
        const std::vector<FMS::ProductionLineSequences> &sequencesHistory) {
    for (const auto &solution : m_solutionEdges) {
        for (const auto &[edge, color] : solution) {
            if (color.isValid()) {
                continue;
            }

            // We need to remove the edge
            auto *const nodeFrom = edge->sourceNode();
            auto *const nodeTo = edge->destNode();

            nodeFrom->removeEdge(edge);
            nodeTo->removeEdge(edge);
            delete edge;
        }
    }

    m_solutionEdges.clear();
    auto boundingBox = sceneRect();

    const auto &nodes = getNodes();

    for (const auto &sequence : sequencesHistory) {
        std::vector<std::tuple<Edge *, QColor>> solutionEdges;

        for (const auto &[moduleId, machineSequences] : sequence) {
            const auto &moduleNodes = nodes.at(moduleId);
            const auto moduleEdges = machineSequences | std::views::values | std::views::join;
            const auto &module = productionLine.getModule(moduleId);
            const auto &dg = module.getDelayGraph();

            for (const auto &[vIdFrom, vIdTo, delay] : moduleEdges) {
                const auto &vFrom = dg.get_vertex(vIdFrom);
                const auto &vTo = dg.get_vertex(vIdTo);

                if (!delayGraph::is_visible(vFrom) || !delayGraph::is_visible(vTo)) {
                    continue;
                }

                const auto &opFrom = vFrom.operation;
                const auto &opTo = vTo.operation;

                auto *const nodeFrom = moduleNodes.at(opFrom);
                auto *const nodeTo = moduleNodes.at(opTo);

                if (dg.has_edge(vFrom, vTo)) {
                    Edge *edge = findEdge(nodeFrom, nodeTo);
                    if (edge == nullptr) {
                        throw std::runtime_error("Edge of dg graph not found in visual graph");
                    }

                    solutionEdges.emplace_back(edge, edge->color());
                }

                // Check if the edge already exists
                Edge *edge = addEdge(nodeFrom, nodeTo, delay, 0, QPen(Qt::green), boundingBox);
                edge->hideWithChildren();
                solutionEdges.emplace_back(edge, QColor());
            }
        }
        m_solutionEdges.push_back(std::move(solutionEdges));
    }

    setSceneRect(boundingBox.adjusted(-kBBoxMargin, -kBBoxMargin, kBBoxMargin, kBBoxMargin));

    m_currentSequenceIndex = -1;
    sequenceSelected(0);
}

void FlowShopVis::ProductionLineGraphWidget::sequenceSelected(std::size_t sequenceIndex) {
    // Ignore invalid values or if the sequence is already displayed
    if (sequenceIndex == m_currentSequenceIndex || sequenceIndex >= m_solutionEdges.size()) {
        return;
    }

    if (m_currentSequenceIndex < m_solutionEdges.size()) {
        // Restore to the state displaying no sequence
        const auto &sequenceCurrent = m_solutionEdges.at(m_currentSequenceIndex);
        for (const auto &[edge, color] : sequenceCurrent) {
            if (color.isValid()) {
                edge->setColor(color);
                edge->adjust();
            } else {
                edge->hideWithChildren();
            }
        }
    }

    m_currentSequenceIndex = sequenceIndex;
    for (const auto &[edge, color] : m_solutionEdges.at(m_currentSequenceIndex)) {
        edge->setColor(Qt::GlobalColor::green);
        edge->showWithChildren();
        edge->adjust();
    }
}
