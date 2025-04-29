#ifndef PRODUCTION_LINE_WIDGET_HPP
#define PRODUCTION_LINE_WIDGET_HPP

#include "production_line_bounds_widget.hpp"
#include "production_line_graph_widget.hpp"

#include <fms/cg/edge.hpp>
#include <fms/problem/indices.hpp>
#include <fms/problem/operation.hpp>
#include <fms/problem/production_line.hpp>

#include <QWidget>

namespace FlowShopVis {
class ProductionLineWidget : public QWidget {
    Q_OBJECT
public:
    explicit ProductionLineWidget(fms::problem::ProductionLine productionLine,
                                  QWidget *parent = nullptr);

signals:
    void showOperation(fms::problem::ModuleId, fms::problem::Operation, fms::cg::VertexId vertexId);

public slots:
    void open_bounds_clicked();

private:
    fms::problem::ProductionLine m_productionLine;

    ProductionLineBoundsWidget *m_boundsWidget;
    ProductionLineGraphWidget *m_graphWidget;
};
} // namespace FlowShopVis

#endif // PRODUCTION_LINE_WIDGET_HPP
