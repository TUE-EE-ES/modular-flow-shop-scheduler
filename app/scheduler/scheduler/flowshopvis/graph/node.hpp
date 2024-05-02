#ifndef NODE_H
#define NODE_H

#include <QGraphicsItem>
#include <QList>

#include <FORPFSSPSD/operation.h>

class QGraphicsSceneMouseEvent;

namespace FlowShopVis {
class Edge;

class Node : public QObject, public QGraphicsEllipseItem {
    Q_OBJECT
public:
    explicit Node(FORPFSSPSD::operation a_operation, qreal radius = 15);

    void addEdge(Edge *edge);
    inline void removeEdge(Edge *edge) {
        m_edgeList.removeAll(edge);
    }

    [[nodiscard]] inline const QList<Edge *> &edges() const { return m_edgeList; }

    enum { Type = UserType + 1 };
    [[nodiscard]] int type() const override { return Type; }

    void calculateForces();

    void setColor(const QColor &color) {
        m_color = color;
        updateGradient();
        update();
    }

    [[nodiscard]] QColor color() const { return m_color; }

    [[nodiscard]] qreal radius() const { return m_radius; }

    /**
     * @brief borderPoint
     * @param angle in radians (between 0 and 2*M_PI)
     * @return
     */
    [[nodiscard]] QPointF borderPoint(qreal angle) const;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    void updateGradient();
private:
    QColor m_color;
    QList<Edge *> m_edgeList;
    const FORPFSSPSD::operation m_operation;
    bool m_pressed;
    const qreal m_radius;
signals:
    void selected(FORPFSSPSD::operation operation);
};
} // namespace FlowShopVis

#endif // NODE_H
