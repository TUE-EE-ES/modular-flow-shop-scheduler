#ifndef BASIC_NODE_H
#define BASIC_NODE_H

#include <QGraphicsItem>
#include <QList>
#include <QRadialGradient>

class QGraphicsSceneMouseEvent;

namespace FlowShopVis {
class Edge;

class BasicNode : public QObject, public QGraphicsItem {
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
public:
    constexpr static const qreal kDefaultRadius = 15;

    /// Constructor for the BasicNode class.
    /// @param radius: The radius of the node. Default is 15.
    explicit BasicNode(qreal radius = kDefaultRadius);

    void addEdge(Edge *edge);

    inline void addEdgeNoAdjust(Edge *edge) { m_edgeList << edge; }

    inline void removeEdge(Edge *edge) { m_edgeList.removeAll(edge); }

    [[nodiscard]] inline const QList<Edge *> &edges() const { return m_edgeList; }

    void setColor(const QColor &color) {
        m_color = color;
        updateGradient();
        update();
    }

    [[nodiscard]] inline QColor color() const { return m_color; }

    [[nodiscard]] inline qreal radius() const { return m_radius; }

    /// @brief Calculate the point on the border of the node at a given angle.
    /// @param angle: The angle in radians (between 0 and 2*M_PI).
    [[nodiscard]] QPointF borderPoint(qreal angle) const;

    enum { Type = UserType + 1 };
    [[nodiscard]] inline int type() const override { return Type; }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QRectF boundingRect() const override;

protected:
    QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value) override;

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    void updateGradient();

private:
    QColor m_color = Qt::yellow;
    bool m_pressed = false;
    qreal m_radius;
    QRadialGradient m_gradient;

    QList<Edge *> m_edgeList;

signals:
    void selected();
};
} // namespace FlowShopVis

#endif // BASIC_NODE_H
