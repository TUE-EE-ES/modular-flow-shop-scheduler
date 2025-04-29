#include "basic_graph_widget.hpp"

#include "basic_node.hpp"

#include <QGraphicsScene>
#include <QOpenGLWidget>
#include <QSurfaceFormat>
#include <QKeyEvent>

using namespace FlowShopVis;

constexpr const QRect kDefaultSceneRect = QRect(-200, -200, 400, 400);
constexpr const QSize kSceneMinimumSize = QSize(200, 400);

BasicGraphWidget::BasicGraphWidget(QWidget *parent) : QGraphicsView(parent) {
    auto *scene = new QGraphicsScene(this);
    scene->setItemIndexMethod(QGraphicsScene::BspTreeIndex);
    scene->setSceneRect(kDefaultSceneRect);

    auto *gl = new QOpenGLWidget(this);
    QSurfaceFormat format;
    format.setSamples(4);
    format.setVersion(3, 3);
    gl->setFormat(format);
    setViewport(gl);

    setScene(scene);

    setCacheMode(CacheModeFlag::CacheBackground);
    setRenderHint(QPainter::Antialiasing);
    setDragMode(DragMode::ScrollHandDrag);
    setOptimizationFlags(QGraphicsView::DontSavePainterState);
    setViewportUpdateMode(ViewportUpdateMode::SmartViewportUpdate);
    setTransformationAnchor(ViewportAnchor::AnchorUnderMouse);
    setMinimumSize(kSceneMinimumSize);
}

void BasicGraphWidget::clear() {
    scene()->clear();
    m_nodes.clear();
}

void BasicGraphWidget::addNode(BasicNode* node, const QPointF& pos, const QColor& color, QRectF& boundingBox) {
    node->setColor(color);
    node->setPos(pos);
    boundingBox |= node->boundingRect().translated(node->pos());
    scene()->addItem(node);
}

void BasicGraphWidget::addNode(BasicNode* node) {
    scene()->addItem(node);
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
