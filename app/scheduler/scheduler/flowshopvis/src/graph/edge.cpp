#include "edge.hpp"

#include "basic_node.hpp"

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <numbers>

using namespace FlowShopVis;

constexpr auto EDGE_PI_3 = std::numbers::pi / 3;
constexpr auto EDGE_SUB_PI_3 = std::numbers::pi - EDGE_PI_3;
constexpr double TO_RADIANS = std::numbers::pi / 180.0;
constexpr double TO_DEGREES = 180.0 * std::numbers::inv_pi;

constexpr const qreal Edge::kLoosenessConst = 1 / (2 * std::numbers::sqrt2) + 0.037;

Edge::Edge(BasicNode *sourceNode,
           BasicNode *destNode,
           const QString &text,
           double angle,
           const QPen &pen) :
    m_source(sourceNode), m_dest(destNode), m_angle(angle * TO_RADIANS), m_text(text), m_pen(pen) {

    setZValue(-1);
    setAcceptedMouseButtons(Qt::NoButton);

    m_source->addEdgeNoAdjust(this);
    m_dest->addEdgeNoAdjust(this);
    refreshLine();

    setPen(m_pen);
}

BasicNode *Edge::sourceNode() const { return m_source; }

BasicNode *Edge::destNode() const { return m_dest; }

void Edge::adjustInternal() {

    if (!m_toAdjust) {
        return;
    }

    m_path.clear();
    if (m_source == nullptr || m_dest == nullptr) {
        return;
    }

    prepareGeometryChange();
    refreshLine();

    if (m_line.length() <= m_source->radius() + m_dest->radius()) {
        return;
    }

    // Emulating a small part of the Tikz behaviour for edges. See PGF/Tikz manual.
    const qreal relativeAngle = m_line.angle() * TO_RADIANS; // radians
    const qreal outAngle = m_angle + relativeAngle;          // radians
    const qreal inAngle = std::numbers::pi + relativeAngle - m_angle;

    const auto borderPointSource = mapFromItem(m_source, m_source->borderPoint(outAngle));
    const auto borderPointDest = mapFromItem(m_dest, m_dest->borderPoint(inAngle));

    m_path.moveTo(borderPointSource);
    if (m_angle == 0) {
        m_path.lineTo(borderPointDest);
    } else {
        const QLineF borderLine(borderPointSource, borderPointDest);
        const qreal ax1 = std::cos(outAngle);
        const qreal ay1 = -std::sin(outAngle);
        const qreal ax2 = std::cos(inAngle);
        const qreal ay2 = -std::sin(inAngle);

        // approximate value of looseness constant that is used in Tikz
        constexpr qreal loosenessConst = 1 / (2 * std::numbers::sqrt2) + 0.037;
        const QPointF cp1 =
                borderPointSource + QPointF(ax1, ay1) * borderLine.length() * loosenessConst;
        const QPointF cp2 =
                borderPointDest + QPointF(ax2, ay2) * borderLine.length() * loosenessConst;

        // Add curve similar to Tikz edge
        m_path.cubicTo(cp1, cp2, borderPointDest);
    }

    m_textPos = m_path.pointAtPercent(0.5);

    constexpr const double arrowSize = 10;
    const QPointF destArrowP1(std::sin(inAngle + EDGE_PI_3) * arrowSize,
                              std::cos(inAngle + EDGE_PI_3) * arrowSize);
    const QPointF destArrowP2(std::sin(inAngle + EDGE_SUB_PI_3) * arrowSize,
                              std::cos(inAngle + EDGE_SUB_PI_3) * arrowSize);

    QPolygonF arrow;
    arrow << borderPointDest << destArrowP1 + borderPointDest << destArrowP2 + borderPointDest;
    m_path.addPolygon(arrow);
    m_path.closeSubpath();

    m_toAdjust = false;
}

QRectF Edge::boundingRect() const {
    // Draw greedy version using a triangle instead of a curve
    QPolygonF triangle;
    triangle << m_line.p1() << m_line.p2();

    if (m_angle != 0) {
        const qreal relativeAngle = m_line.angle() * TO_RADIANS; // radians
        const qreal outAngle = m_angle + relativeAngle;          // radians

        const qreal ax1 = std::cos(outAngle);
        const qreal ay1 = -std::sin(outAngle);

        // approximate value of looseness constant that is used in Tikz
        const QPointF cp1 = m_line.p1() + QPointF(ax1, ay1) * m_line.length() * kLoosenessConst;
        triangle << cp1;
    }

    const auto baseRect = triangle.boundingRect();
    return baseRect.adjusted(-kSpacing, -kSpacing, kSpacing, kSpacing);
}

void FlowShopVis::Edge::paint(QPainter *painter,
                              const QStyleOptionGraphicsItem *option,
                              QWidget *widget) {
    Q_UNUSED(widget);

    adjustInternal();

    const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

    if (m_path.isEmpty()) {
        return;
    }

    painter->setPen(m_pen);
    painter->drawPath(m_path);

    if (lod >= 0.7) {
        const auto size = m_text.size();
        painter->drawStaticText(m_textPos - QPointF(size.width() / 2, size.height() / 2), m_text);
    }
}

void Edge::refreshLine() {
    m_line = QLineF(mapFromItem(m_source, 0, 0), mapFromItem(m_dest, 0, 0));
}
