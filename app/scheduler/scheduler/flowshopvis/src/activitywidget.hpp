#ifndef ACTIVITYWIDGET_H
#define ACTIVITYWIDGET_H

#include <QGraphicsView>

#include <fms/problem/flow_shop.hpp>

#include "graph/edge.hpp"

namespace FlowShopVis {

class FlowShopGraphWidget;

class ActivityWidget : public QGraphicsView
{
    Q_OBJECT
public:
    explicit ActivityWidget(FlowShopGraphWidget &graphwidget, QWidget *parent = nullptr);

    void openSolution(const fms::solvers::PartialSolution &solution, const fms::problem::Instance &instance);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void scaleView(qreal scaleFactor);

private:
    const QVector<Edge *> sequence_edges;

    FlowShopGraphWidget & graphwidget;
};
} // namespace FlowShopVis
#endif // ACTIVITYWIDGET_H
