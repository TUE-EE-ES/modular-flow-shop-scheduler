#ifndef BASIC_GRAPH_WIDGET_HPP
#define BASIC_GRAPH_WIDGET_HPP

#include "edge.hpp"

#include <QGraphicsView>
#include <QString>

// NOLINTBEGIN(readability-redundant-access-specifiers)

namespace FlowShopVis {

class BasicNode;
class Edge;

class BasicGraphWidget : public QGraphicsView {
    Q_OBJECT
public:
    constexpr static const qreal kDefaultMargin = 500;

    explicit BasicGraphWidget(QWidget *parent = nullptr);

    virtual void clear();

    void addNode(BasicNode *node, const QPointF &pos, const QColor &color, QRectF &boundingBox);

    void addNode(BasicNode *node);

    inline void addEdge(Edge *edge, QRectF &boundingBox) {
        boundingBox |= edge->boundingRect();
        scene()->addItem(edge);
    }

    inline static QRectF adjustMargin(QRectF rect, qreal margin = kDefaultMargin) {
        return rect.adjusted(-margin, -margin, margin, margin);
    }

public slots:
    /// @brief Zooms in the view
    void zoomIn();

    /// @brief Zooms out the view
    void zoomOut();

protected:
    void keyPressEvent(QKeyEvent *event) override;

    void wheelEvent(QWheelEvent *event) override;

    void scaleView(qreal scaleFactor);

private:
    QList<BasicNode *> m_nodes;
};

} // namespace FlowShopVis

// NOLINTEND(readability-redundant-access-specifiers)

#endif // BASIC_GRAPH_WIDGET_HPP