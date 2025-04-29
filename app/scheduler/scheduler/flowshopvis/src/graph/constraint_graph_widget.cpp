#include "constraint_graph_widget.hpp"

#include "edge.hpp"
#include "operation_node.hpp"

using namespace FlowShopVis;

constexpr static const qreal kOpXPos = 100;
constexpr static const qreal kOpYPos = 200;

const QList<QColor> ConstraintGraphWidget::colors = {Qt::cyan,
                                                     Qt::magenta,
                                                     Qt::yellow,
                                                     Qt::red,
                                                     Qt::blue,
                                                     Qt::green,
                                                     QColor("orange"),
                                                     Qt::gray,
                                                     QColor("violet"),
                                                     QColor("purple")};

ConstraintGraphWidget::ConstraintGraphWidget(QWidget *parent) : BasicGraphWidget(parent) {}

ConstraintGraphWidget::ConstraintGraphWidget(const fms::cg::ConstraintGraph &dg,
                                             const DOT::ColouredEdges &highlighted,
                                             QWidget *parent) :
    BasicGraphWidget(parent) {

    QRectF boundingBox;
    const fms::problem::ModuleId moduleId{0};

    // add all vertices
    for (const fms::cg::Vertex &v : dg.getVertices()) {
        if (!fms::cg::ConstraintGraph::isVisible(v)) {
            continue;
        }

        const auto &op = v.operation;
        QPointF pos(kOpXPos * op.jobId.value, kOpYPos * op.operationId);
        QColor color(getColor(v.operation.operationId));

        addNode(moduleId, op, v.id, pos, color, boundingBox);
    }

    addModuleEdges(moduleId, dg, boundingBox, highlighted);
    setSceneRect(adjustMargin(boundingBox));
}

OperationNode *ConstraintGraphWidget::addNode(fms::problem::ModuleId moduleId,
                                              fms::problem::Operation operation,
                                              fms::cg::VertexId vertexId,
                                              const QPointF &pos,
                                              const QColor &color,
                                              QRectF &boundingBox) {

    auto *node = new OperationNode(operation, vertexId);
    m_nodes[moduleId][operation] = node;

    BasicGraphWidget::addNode(node, pos, color, boundingBox);

    connect(node,
            &OperationNode::operationSelected,
            this,
            [this, moduleId](fms::problem::Operation operation, fms::cg::VertexId vertexId) {
                emit showOperation(moduleId, operation, vertexId);
            });
    scene()->addItem(node);
    return node;
}

// Overloaded function to add a node with ASAP and ALAP times
OperationNode *ConstraintGraphWidget::addNode(fms::problem::ModuleId moduleId,
                                              fms::problem::Operation operation,
                                              fms::cg::VertexId vertexId,
                                              const QPointF &pos,
                                              const QColor &color,
                                              QRectF &boundingBox,
                                              fms::delay ASAP,
                                              fms::delay ALAP) {

    auto *node = new OperationNode(operation, vertexId);
    node->setASAP(ASAP);
    node->setALAP(ALAP);
    m_nodes[moduleId][operation] = node;

    node->setColor(color);
    node->setPos(pos);
    boundingBox |= node->boundingRect().translated(node->pos());

    connect(node,
            &OperationNode::operationSelected,
            this,
            [this, moduleId](fms::problem::Operation operation, fms::cg::VertexId vertexId) {
                emit showOperation(moduleId, operation, vertexId);
            });
    scene()->addItem(node);
    return node;
}

Edge *ConstraintGraphWidget::addEdge(OperationNode *source,
                                     OperationNode *dest,
                                     fms::delay weight,
                                     double angle,
                                     const QPen &pen,
                                     QRectF &boundingBox) {

    auto *edge = new Edge(source, dest, QString::number(weight), angle, pen);
    BasicGraphWidget::addEdge(edge, boundingBox);
    return edge;
}

void ConstraintGraphWidget::addModuleEdges(fms::problem::ModuleId moduleId,
                                           const fms::cg::ConstraintGraph &dg,
                                           QRectF &boundingbox,
                                           const DOT::ColouredEdges &highlighted) {
    const auto &nodes = getNodes().at(moduleId);
    for (const fms::cg::Vertex &v : dg.getVertices()) {
        for (const auto &[dst, weight] : v.getOutgoingEdges()) {
            const auto &vDst = dg.getVertex(dst);

            if (fms::cg::ConstraintGraph::isSource(v) || fms::cg::ConstraintGraph::isSource(vDst)) {
                continue;
            }

            const auto &opSrc = v.operation;
            const auto &opDst = vDst.operation;
            std::uint32_t bend = 0;

            QPen pen;
            if (weight < 0) {
                // deadlines are bent, red
                bend = 30;
                pen.setColor(Qt::darkRed);
                pen.setStyle(Qt::DotLine);
            }

            if (opSrc.jobId != opDst.jobId) {
                // Different jobs, blue
                pen.setColor(Qt::blue);
            }

            // Find the colour
            const auto it = highlighted.find(v.id);
            if (it != highlighted.end()) {
                const auto it2 = it->second.find(dst);
                if (it2 != it->second.end()) {
                    pen.setColor(it2->second);
                }
            }

            addEdge(nodes.at(opSrc), nodes.at(opDst), weight, bend, pen, boundingbox);
        }
    }
}
