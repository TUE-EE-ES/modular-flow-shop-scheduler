#ifndef FLOWSHOPWIDGET_H
#define FLOWSHOPWIDGET_H

#include <QWidget>
#include <FORPFSSPSD/FORPFSSPSD.h>

namespace FlowShopVis {
    class GraphWidget;
    class ActivityWidget;
}

class FlowshopWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FlowshopWidget(FORPFSSPSD::Instance instance, QWidget *parent = nullptr);

signals:
    void showOperation(FORPFSSPSD::ModuleId, FORPFSSPSD::operation);
public slots:
    void open_asapst_clicked();
    void open_sequence_clicked();
private:

    FlowShopVis::GraphWidget * graphWidget;
    FlowShopVis::ActivityWidget * activityWidget;
    FORPFSSPSD::Instance m_instance;
};

#endif // FLOWSHOPWIDGET_H
