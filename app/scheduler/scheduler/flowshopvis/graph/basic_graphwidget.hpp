#ifndef BASIC_GRAPHWIDGET_NO_FLOWSHOP_H
#define BASIC_GRAPHWIDGET_NO_FLOWSHOP_H

#include "dot_parser.hpp"

#include <FORPFSSPSD/FORPFSSPSD.h>

#include <QGraphicsView>
#include <QString>
#include <QWidget>

namespace FlowShopVis {
class Node;
class Edge;

class BasicGraphWidget : public QGraphicsView {
    Q_OBJECT
public:
    explicit BasicGraphWidget(QWidget *parent = nullptr);

    static const QList<QColor> colors;

    [[nodiscard]] inline const auto& getNodes() { return m_nodes; }
    Node *addNode(FORPFSSPSD::ModuleId moduleId,
                  FORPFSSPSD::operation operation,
                  const QPointF &pos,
                  const QColor &color,
                  QRectF &boundingBox);

    Edge *addEdge(Node *source,
                  Node *dest,
                  delay weight,
                  double angle,
                  const QPen &pen,
                  QRectF &boundingBox);

    void addModuleEdges(FORPFSSPSD::ModuleId moduleId,
                        const DelayGraph::delayGraph &dg,
                        QRectF &boundingbox,
                        const DOT::ColouredEdges &highlighted = {});

    [[nodiscard]] inline const auto &getNode(FORPFSSPSD::ModuleId moduleId,
                                             FORPFSSPSD::operation operation) {
        return m_nodes.at(moduleId).at(operation);
    }

    [[nodiscard]] static QColor getColor(std::size_t machine) {
        return colors[machine % colors.size()];
    }

signals:
    void showOperation(FORPFSSPSD::ModuleId, FORPFSSPSD::operation);

public slots:
    void zoomIn();
    void zoomOut();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void scaleView(qreal scaleFactor);

private:
    FMS::Map<FORPFSSPSD::ModuleId, FMS::Map<FORPFSSPSD::operation, Node *>> m_nodes;
};
} // namespace FlowShopVis
#endif // BASIC_GRAPHWIDGET_NO_FLOWSHOP_H
