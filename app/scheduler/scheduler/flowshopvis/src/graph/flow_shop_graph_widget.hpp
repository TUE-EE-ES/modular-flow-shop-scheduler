#ifndef FLOW_SHOP_GRAPH_WIDGET_HPP
#define FLOW_SHOP_GRAPH_WIDGET_HPP

#include "constraint_graph_widget.hpp"

#include <fms/algorithms/longest_path.hpp>
#include <fms/problem/flow_shop.hpp>
#include <fms/solvers/partial_solution.hpp>

#include <QString>
#include <QWidget>
#include <fmt/core.h>

namespace FlowShopVis {
class Node;

class FlowShopGraphWidget : public ConstraintGraphWidget {
    Q_OBJECT
public:
    // Constructor for the FlowShopGraphWidget class.
    // @param instance: A reference to an instance of the FS::Instance class.
    // @param parent: A pointer to the parent QWidget. Default is nullptr.
    explicit FlowShopGraphWidget(const fms::problem::Instance &instance,
                                 QWidget *parent = nullptr,
                                 const fms::algorithms::paths::PathTimes &ASAPST = {},
                                 const fms::algorithms::paths::PathTimes &ALAPST = {});

    // The setPartialSolution function sets the partial solution for the graph widget.
    // @param ps: A reference to a PartialSolution object.
    // @param instance: A reference to an instance of the FS::Instance class.
    void setPartialSolution(const fms::solvers::PartialSolution &ps,
                            const fms::problem::Instance &instance);

    QRectF setPartialSequence(const fms::solvers::PartialSolution &ps,
                              const fms::problem::Instance &instance);

    QRectF setSequences(const fms::solvers::MachinesSequences &machinesSequences,
                        const fms::problem::Instance &instance);

private:
    std::vector<Edge *> m_currentSequenceEdges;
};
} // namespace FlowShopVis
#endif // GRAPHWIDGET_H
