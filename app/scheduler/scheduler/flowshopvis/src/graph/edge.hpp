#ifndef EDGE_H
#define EDGE_H

#include <fms/delay.hpp>

#include <QGraphicsItem>
#include <QPainterPath>
#include <QPen>
#include <QStaticText>

namespace FlowShopVis {

class BasicNode;

class Edge : public QGraphicsItem {
    Q_INTERFACES(QGraphicsItem)
public:
    constexpr static const qreal kDefaultAngle = 0;
    constexpr static const qreal kArrowSize = 10;
    static const qreal kLoosenessConst;
    constexpr static const qreal kMargin = 10;
    constexpr static const qreal kSpacing = kArrowSize + kMargin;

    Edge(BasicNode *sourceNode,
         BasicNode *destNode,
         const QString &text = QString(),
         double angle = kDefaultAngle,
         const QPen &pen = QPen());

    [[nodiscard]] BasicNode *sourceNode() const;
    [[nodiscard]] BasicNode *destNode() const;

    [[nodiscard]] inline QColor color() const { return m_pen.color(); }
    inline void setColor(const QColor &color) {
        m_pen.setColor(color);
        setPen(m_pen);
        // update();
    }

    inline void setPen(const QPen &pen) { m_pen = pen; }

    enum { Type = UserType + 2 };
    [[nodiscard]] inline int type() const override { return Type; }

    void setText(const QString &text) {
        m_text.setText(text);
        update();
    }

    inline void adjust() noexcept { m_toAdjust = true; }

    QRectF boundingRect() const override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    BasicNode *m_source, *m_dest;
    double m_angle;
    QPen m_pen;
    QPainterPath m_path;
    QStaticText m_text;
    QPointF m_textPos;

    QLineF m_line;
    bool m_toAdjust = true;

    void refreshLine();
    void adjustInternal();
};
} // namespace FlowShopVis
#endif
