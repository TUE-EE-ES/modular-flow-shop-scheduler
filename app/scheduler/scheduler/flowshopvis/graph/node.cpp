#define _USE_MATH_DEFINES

#include "node.hpp"
#include "edge.hpp"

#include "graphwidget.hpp"

#include <QPainter>
using namespace FlowShopVis;

Node::Node(FORPFSSPSD::operation a_operation, qreal radius) :
    QGraphicsEllipseItem(-radius, -radius, 2 * radius, 2 * radius),
    m_radius(radius),
    m_operation(a_operation),
    m_pressed(false),
    m_color(Qt::yellow) {
    setFlag(ItemIsMovable);
    setFlag(ItemIsSelectable);
    setFlag(ItemSendsGeometryChanges);
    setCacheMode(DeviceCoordinateCache);
    setZValue(-1);
    setPen(QPen(Qt::black, 0));
    updateGradient();
}

void Node::addEdge(Edge *edge) {
    m_edgeList << edge;
    edge->adjust();
}

QVariant Node::itemChange(GraphicsItemChange change, const QVariant &value) {
    switch (change) {
    case ItemPositionHasChanged:
        for (auto *edge : m_edgeList) {
            edge->adjust();
        }
        break;
    default:
        break;
    }

    return QGraphicsEllipseItem::itemChange(change, value);
}
void Node::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    m_pressed = true;

    emit selected(m_operation);

    updateGradient();
    update();
    QGraphicsEllipseItem::mousePressEvent(event);
}

void Node::updateGradient() {
    QRadialGradient gradient(-0, -0, m_radius);

    if (m_pressed) {
        gradient.setColorAt(1, m_color.lighter(120));
        gradient.setColorAt(0, m_color.darker().lighter(120));
    } else {
        gradient.setColorAt(0, m_color);
        gradient.setColorAt(1, m_color.darker());
    }
    QGraphicsEllipseItem::setBrush(gradient);
}

void Node::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    m_pressed = false;
    updateGradient();
    update();
    QGraphicsEllipseItem::mouseReleaseEvent(event);
}
QPointF FlowShopVis::Node::borderPoint(qreal angle) const {
    return {std::cos(angle) * m_radius, -std::sin(angle) * m_radius};
}
