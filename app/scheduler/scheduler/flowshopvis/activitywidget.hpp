#ifndef ACTIVITYWIDGET_H
#define ACTIVITYWIDGET_H

#include <QGraphicsView>

#include <FORPFSSPSD/FORPFSSPSD.h>

#include "graph/edge.hpp"

namespace FlowShopVis {

class GraphWidget;

class ActivityWidget : public QGraphicsView
{
    Q_OBJECT
public:
    explicit ActivityWidget(GraphWidget &graphwidget, QWidget *parent = nullptr);

protected:
#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent *event) override;
#endif
    void scaleView(qreal scaleFactor);
signals:

public slots:
    void open_asapst_file(const QString &fileName, const FORPFSSPSD::Instance &instance);

private:
    const QVector<Edge *> sequence_edges;

    GraphWidget & graphwidget;
};
} // namespace FlowShopVis
#endif // ACTIVITYWIDGET_H
