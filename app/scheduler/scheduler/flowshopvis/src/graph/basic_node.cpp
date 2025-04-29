#include "basic_node.hpp"

#include "edge.hpp"

#include <QPainter>
#include <QPen>
#include <QStyleOptionGraphicsItem>

using namespace FlowShopVis;

BasicNode::BasicNode(qreal radius) : QGraphicsItem(), m_radius(radius), m_gradient(0, 0, radius) {
    setFlag(ItemIsMovable);
    setFlag(ItemIsSelectable);
    setFlag(ItemSendsGeometryChanges);
    setCacheMode(DeviceCoordinateCache);
    updateGradient();
}

void BasicNode::addEdge(Edge *edge) {
    m_edgeList << edge;
    edge->adjust();
}

QPointF BasicNode::borderPoint(qreal angle) const {
    return {std::cos(angle) * m_radius, -std::sin(angle) * m_radius};
}

void BasicNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(widget);

    const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

    if (lod >= 0.5) {
        painter->setBrush(m_gradient);
    } else {
        painter->setBrush(m_pressed ? m_color.darker() : m_color);
    }
    painter->drawEllipse(QPointF(0, 0), m_radius, m_radius);
}

QRectF BasicNode::boundingRect() const {
    constexpr const qreal kSpacing = 10;

    return QRectF(-m_radius - kSpacing,
                  -m_radius - kSpacing,
                  2 * (m_radius + kSpacing),
                  2 * (m_radius + kSpacing));
}

QVariant BasicNode::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value) {
    switch (change) {
    case ItemPositionHasChanged:
        for (auto *edge : m_edgeList) {
            edge->adjust();
        }
        break;
    default:
        break;
    }

    return QGraphicsItem::itemChange(change, value);
}

void BasicNode::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    m_pressed = true;

    emit selected();

    updateGradient();
    QGraphicsItem::mousePressEvent(event);
}

void BasicNode::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    m_pressed = false;
    updateGradient();
    QGraphicsItem::mouseReleaseEvent(event);
}

void BasicNode::updateGradient() {
    constexpr const std::int32_t kLighterFactor = 120;

    if (m_pressed) {
        m_gradient.setColorAt(1, m_color.lighter(kLighterFactor));
        m_gradient.setColorAt(0, m_color.darker().lighter(kLighterFactor));
    } else {
        m_gradient.setColorAt(0, m_color);
        m_gradient.setColorAt(1, m_color.darker());
    }
    update();
}
