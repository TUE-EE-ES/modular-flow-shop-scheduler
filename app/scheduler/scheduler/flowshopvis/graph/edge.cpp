#define _USE_MATH_DEFINES

#include "edge.hpp"

#include "node.hpp"

#include <QPainter>
#include <numbers>

using namespace FlowShopVis;

constexpr auto EDGE_PI_3 = std::numbers::pi / 3;
constexpr auto EDGE_SUB_PI_3 = std::numbers::pi - EDGE_PI_3;
constexpr double TO_RADIANS = std::numbers::pi / 180.0;
constexpr double TO_DEGREES = 180.0 * std::numbers::inv_pi;

Edge::Edge(Node *sourceNode, Node *destNode, delay weight, double angle, const QPen &pen) :
    source(sourceNode),
    dest(destNode),
    _angle(angle * TO_RADIANS),
    path(new QGraphicsPathItem(this)),
    arrowHead(new QGraphicsPolygonItem(this)),
    textItem(new QGraphicsTextItem(this)) {
    
    setAcceptedMouseButtons(Qt::NoButton);
    source->addEdge(this);
    dest->addEdge(this);
    setPen(pen);
    setText(QString::number(weight));

    const double arrowSize = 10;
    QPointF destArrowP1 = QPointF(sin(EDGE_PI_3) * arrowSize, cos(EDGE_PI_3) * arrowSize);
    QPointF destArrowP2 = QPointF(sin(EDGE_SUB_PI_3) * arrowSize, cos(EDGE_SUB_PI_3) * arrowSize);

    arrowHead->setPolygon(QPolygonF() << QPointF(0, 0) << destArrowP1 << destArrowP2);
    arrowHead->setBrush(Qt::white);
    adjust();
}

Node *Edge::sourceNode() const
{
    return source;
}

Node *Edge::destNode() const
{
    return dest;
}

void Edge::adjust()
{
    if (!isVisible()) {
        return;
    }
    
    prepareGeometryChange();

    if (source == nullptr || dest == nullptr) {
        path->setPath(QPainterPath());
        return;
    }

    QLineF line(mapFromItem(source, 0, 0), mapFromItem(dest, 0, 0));
    qreal length = line.length();

    if (length <= static_cast<qreal>(source->radius() + dest->radius())) {
        path->setPath(QPainterPath()); // no line
        return;
    }
    // Emulating a small part of the Tikz behaviour for edges. See PGF/Tikz manual.

    qreal relative_angle = line.angle() * TO_RADIANS; // radians

    qreal out_angle = _angle + relative_angle; // radians
    qreal in_angle = std::numbers::pi + relative_angle - _angle;

    QPainterPath edge;
    QPointF b1 =
            mapFromItem(source, source->borderPoint(out_angle)); // border point of the source node
    QPointF b2 = mapFromItem(dest, dest->borderPoint(in_angle)); // border point of the target node

    edge.moveTo(b1);

    QLineF border_line(b1, b2);
    qreal ax1 = cos(out_angle);
    qreal ay1 = -sin(out_angle);
    qreal ax2 = cos(in_angle);
    qreal ay2 = -sin(in_angle);

    // approximate value of looseness constant that is used in Tikz
    constexpr qreal looseness_const = 1 / (2 * std::numbers::sqrt2) + 0.037;
    QPointF cp1 = b1 + QPointF(ax1, ay1) * border_line.length() * looseness_const;
    QPointF cp2 = b2 + QPointF(ax2, ay2) * border_line.length() * looseness_const;

    // Add curve similar to Tikz edge
    edge.cubicTo(cp1, cp2, b2);

    path->setPath(edge);
    textItem->setPos(
            edge.pointAtPercent(0.5)
            - QPointF(textItem->boundingRect().width() / 2, textItem->boundingRect().height() / 2));
    textItem->setDefaultTextColor(_pen.color());
    path->setPen(_pen);

    QPen pen(_pen);
    pen.setStyle(Qt::SolidLine);
    pen.setJoinStyle(Qt::MiterJoin);
    pen.setCapStyle(Qt::SquareCap);
    arrowHead->setPos(b2);
    arrowHead->setRotation(-in_angle * TO_DEGREES);
    arrowHead->setPen(pen);
}

QRectF Edge::boundingRect() const { return path->boundingRect().united(textItem->boundingRect()); }

void FlowShopVis::Edge::paint(QPainter *painter,
                              const QStyleOptionGraphicsItem *option,
                              QWidget *widget) {
    Q_UNUSED(painter)
    Q_UNUSED(option)
    Q_UNUSED(widget)
}
