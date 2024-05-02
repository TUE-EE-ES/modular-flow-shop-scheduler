#ifndef PRODUCTION_LINE_GRAPH_WIDGET_HPP
#define PRODUCTION_LINE_GRAPH_WIDGET_HPP

#include "graph/basic_graphwidget.hpp"

#include <FORPFSSPSD/production_line.hpp>
#include <solvers/production_line_solution.hpp>

namespace FlowShopVis {
class ProductionLineGraphWidget : public BasicGraphWidget {
    Q_OBJECT
public:
    explicit ProductionLineGraphWidget(const FORPFSSPSD::ProductionLine &productionLine,
                                       QWidget *parent = nullptr);

    void setSequencesHistory(const FORPFSSPSD::ProductionLine &productionLine,
                             const std::vector<FMS::ProductionLineSequences> &sequencesHistory);

public slots:
    void sequenceSelected(std::size_t sequenceIndex);

private:
    std::vector<std::vector<std::tuple<Edge *, QColor>>> m_solutionEdges;
    std::size_t m_currentSequenceIndex = 0;
};
} // namespace FlowShopVis

#endif // PRODUCTION_LINE_GRAPH_WIDGET_HPP