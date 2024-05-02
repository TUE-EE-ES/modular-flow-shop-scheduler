#include "basic_graphwidget.hpp"

#include "edge.hpp"
#include "node.hpp"

#include <QKeyEvent>
#include <QOpenGLWidget>
#include <gsl/gsl>

using namespace FlowShopVis;
using namespace DelayGraph;

constexpr QRect kSceneRect = QRect(-200, -200, 400, 400);
constexpr QSize kMinimumSize = QSize(200, 400);

const QList<QColor> BasicGraphWidget::colors = {Qt::cyan,
                                                Qt::magenta,
                                                Qt::yellow,
                                                Qt::red,
                                                Qt::blue,
                                                Qt::green,
                                                QColor("orange"),
                                                Qt::gray,
                                                QColor("violet"),
                                                QColor("purple")};

BasicGraphWidget::BasicGraphWidget(QWidget *parent) : QGraphicsView(parent) {

    auto *scene = new QGraphicsScene(this);
    scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    scene->setSceneRect(kSceneRect);

    auto *gl = new QOpenGLWidget(this);
    QSurfaceFormat format;
    format.setSamples(4);
    gl->setFormat(format);
    setViewport(gl);

    setScene(scene);

    setCacheMode(CacheModeFlag::CacheBackground);
    setViewportUpdateMode(ViewportUpdateMode::BoundingRectViewportUpdate);
    setRenderHint(QPainter::Antialiasing);
    setTransformationAnchor(ViewportAnchor::AnchorUnderMouse);
    setMinimumSize(kMinimumSize);
    setDragMode(DragMode::ScrollHandDrag);
}

Node *BasicGraphWidget::addNode(FORPFSSPSD::ModuleId moduleId,
                                FORPFSSPSD::operation operation,
                                const QPointF &pos,
                                const QColor &color,
                                QRectF &boundingBox) {

    auto *node = new Node(operation);
    m_nodes[moduleId][operation] = node;

    node->setColor(color);
    node->setPos(pos);
    boundingBox |= node->boundingRect().translated(node->pos());

    connect(node,
            &Node::selected,
            this,
            [this, moduleId](FORPFSSPSD::operation operation) {
                emit showOperation(moduleId, operation);
            });
    scene()->addItem(node);
    return node;
}

Edge *BasicGraphWidget::addEdge(Node *source,
                                Node *dest,
                                delay weight,
                                double angle,
                                const QPen &pen,
                                QRectF &boundingBox) {
    auto *edge = new Edge(source, dest, weight, angle, pen);
    boundingBox |= edge->boundingRect();
    scene()->addItem(edge);
    return edge;
}

void BasicGraphWidget::addModuleEdges(FORPFSSPSD::ModuleId moduleId,
                                      const DelayGraph::delayGraph &dg,
                                      QRectF &boundingbox,
                                      const DOT::ColouredEdges &highlighted) {
    const auto &nodes = getNodes().at(moduleId);
    for (const DelayGraph::vertex &v : dg.get_vertices()) {
        for (const auto &[dst, weight] : v.get_outgoing_edges()) {
            const auto &vDst = dg.get_vertex(dst);

            if (delayGraph::is_source(v) || delayGraph::is_source(vDst)) {
                continue;
            }

            const auto &opSrc = v.operation;
            const auto &opDst = dg.get_vertex(dst).operation;
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

void BasicGraphWidget::zoomIn() { scaleView(qreal(1.2)); }

void BasicGraphWidget::zoomOut() { scaleView(1 / qreal(1.2)); }

void BasicGraphWidget::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
    case Qt::Key_Plus:
        zoomIn();
        break;
    case Qt::Key_Minus:
        zoomOut();
        break;
    default:
        QGraphicsView::keyPressEvent(event);
    }
}

void BasicGraphWidget::scaleView(qreal scaleFactor) {
    qreal factor = transform().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
    if (factor < 0.07 || factor > 100) {
        return;
    }

    scale(scaleFactor, scaleFactor);
}

void BasicGraphWidget::wheelEvent(QWheelEvent *event) {
    scaleView(std::pow((double)2, event->angleDelta().y() / 240.0));
}
