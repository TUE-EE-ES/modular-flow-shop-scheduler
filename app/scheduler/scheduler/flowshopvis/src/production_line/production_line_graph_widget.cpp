#include "production_line_graph_widget.hpp"

#include "graph/edge.hpp"
#include "graph/operation_node.hpp"

using namespace FlowShopVis;

namespace {
Edge *findEdge(OperationNode *nodeFrom, OperationNode *nodeTo) {
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
        const fms::problem::ProductionLine &productionLine, QWidget *parent) :
    ConstraintGraphWidget(parent) {

    // Assume that all modules have their graph initialized

    const auto &moduleIds = productionLine.moduleIds();

    QRectF boundingBox;
    QPointF offset(0, 0);

    std::size_t machineIndex = 0;
    for (const auto moduleId : moduleIds) {
        const auto &module = productionLine.getModule(moduleId);
        std::unordered_map<fms::problem::MachineId, std::size_t> machineToIndex;
        for (const auto &machineId : module.getMachines()) {
            machineToIndex[machineId] = machineIndex++;
        }

        const auto &dg = module.getDelayGraph();

        // Add all vertices
        double maxY = 0;
        for (const fms::cg::Vertex &v : dg.getVertices()) {
            if (!fms::cg::ConstraintGraph::isVisible(v)) {
                continue;
            }
            const auto &op = v.operation;
            QPointF pos(100 * op.jobId.value, 200 * op.operationId);
            pos += offset;
            maxY = std::max(maxY, pos.y());

            QColor color(getColor(machineToIndex.at(module.getMachine(op))));

            addNode(moduleId, op, v.id, pos, color, boundingBox);
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

                fms::delay time = moduleFrom.getProcessingTime(opFrom) + point.setupTime(jobId);
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
        const fms::problem::ProductionLine &productionLine,
        const std::vector<fms::solvers::ProductionLineEdges> &sequencesHistory) {
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
            scene()->removeItem(edge);
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
                const auto &vFrom = dg.getVertex(vIdFrom);
                const auto &vTo = dg.getVertex(vIdTo);

                if (!fms::cg::ConstraintGraph::isVisible(vFrom)
                    || !fms::cg::ConstraintGraph::isVisible(vTo)) {
                    continue;
                }

                const auto &opFrom = vFrom.operation;
                const auto &opTo = vTo.operation;

                auto *const nodeFrom = moduleNodes.at(opFrom);
                auto *const nodeTo = moduleNodes.at(opTo);

                if (dg.hasEdge(vFrom, vTo)) {
                    Edge *edge = findEdge(nodeFrom, nodeTo);
                    if (edge == nullptr) {
                        throw std::runtime_error("Edge of dg graph not found in visual graph");
                    }

                    solutionEdges.emplace_back(edge, edge->color());
                }

                // Check if the edge already exists
                Edge *edge = addEdge(nodeFrom, nodeTo, delay, 0, QPen(Qt::green), boundingBox);
                edge->hide();
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
                edge->hide();
            }
        }
    }

    m_currentSequenceIndex = sequenceIndex;
    for (const auto &[edge, color] : m_solutionEdges.at(m_currentSequenceIndex)) {
        edge->setColor(Qt::GlobalColor::green);
        edge->show();
        edge->adjust();
    }
}
