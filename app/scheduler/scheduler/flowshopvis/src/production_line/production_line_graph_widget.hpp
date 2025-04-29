#ifndef PRODUCTION_LINE_GRAPH_WIDGET_HPP
#define PRODUCTION_LINE_GRAPH_WIDGET_HPP

#include "graph/constraint_graph_widget.hpp"

#include <fms/problem/production_line.hpp>
#include <fms/solvers/production_line_solution.hpp>

namespace FlowShopVis {
class ProductionLineGraphWidget : public ConstraintGraphWidget {
    Q_OBJECT
public:
    explicit ProductionLineGraphWidget(const fms::problem::ProductionLine &productionLine,
                                       QWidget *parent = nullptr);

    void
    setSequencesHistory(const fms::problem::ProductionLine &productionLine,
                        const std::vector<fms::solvers::ProductionLineEdges> &sequencesHistory);

public slots:
    void sequenceSelected(std::size_t sequenceIndex);

private:
    std::vector<std::vector<std::tuple<Edge *, QColor>>> m_solutionEdges;
    std::size_t m_currentSequenceIndex = 0;
};

} // namespace FlowShopVis

#endif // PRODUCTION_LINE_GRAPH_WIDGET_HPP