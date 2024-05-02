#ifndef PRODUCTION_LINE_WIDGET_HPP
#define PRODUCTION_LINE_WIDGET_HPP

#include "production_line_bounds_widget.hpp"
#include "production_line_graph_widget.hpp"

#include <FORPFSSPSD/indices.hpp>
#include <FORPFSSPSD/operation.h>
#include <FORPFSSPSD/production_line.hpp>

#include <QWidget>

namespace FlowShopVis {
class ProductionLineWidget : public QWidget {
    Q_OBJECT
public:
    explicit ProductionLineWidget(FORPFSSPSD::ProductionLine productionLine,
                                  QWidget *parent = nullptr);

signals:
    void showOperation(FORPFSSPSD::ModuleId, FORPFSSPSD::operation);

public slots:
    void open_bounds_clicked();

private:
    FORPFSSPSD::ProductionLine m_productionLine;

    ProductionLineBoundsWidget *m_boundsWidget;
    ProductionLineGraphWidget *m_graphWidget;
};
} // namespace FlowShopVis

#endif // PRODUCTION_LINE_WIDGET_HPP
