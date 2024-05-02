#ifndef EDGE_H
#define EDGE_H

#include <delay.h>

#include <QGraphicsItem>
#include <QPen>

namespace FlowShopVis {

class Node;

class Edge : public QGraphicsItem
{
public:
    static constexpr double kDefaultAngle = 30;

    Edge(Node *sourceNode, Node *destNode, delay weight, double angle = kDefaultAngle, const QPen &pen = QPen());

    [[nodiscard]] Node *sourceNode() const;
    [[nodiscard]] Node *destNode() const;

    void adjust();

    [[nodiscard]] inline QColor color() const { return _pen.color(); }
    inline void setColor(const QColor &color) {
        _pen.setColor(color);
        update();
    }

    enum { Type = UserType + 2 };
    [[nodiscard]] int type() const override { return Type; }
    void setPen(const QPen & pen) {
        _pen = pen;
        _pen.setCapStyle(Qt::FlatCap);
        adjust();
        update();
    }

    void setText(const QString & text) {
        textItem->setPlainText(text);
        adjust();
        update();
    }

    [[nodiscard]] QRectF boundingRect() const override;

    inline void hideWithChildren() {
        hide();
        path->hide();
        arrowHead->hide();
        textItem->hide();
    }

    inline void showWithChildren() {
        show();
        path->show();
        arrowHead->show();
        textItem->show();
    }

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    Node *source, *dest;
    double _angle;
    QPointF cp1, cp2;
    QGraphicsPathItem * path;
    QGraphicsPolygonItem *arrowHead;
    QPen _pen;
    QGraphicsTextItem * textItem;
};
} // namespace FlowShopVis
#endif
