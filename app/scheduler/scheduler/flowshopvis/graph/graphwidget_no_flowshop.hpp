#ifndef GRAPHWIDGET_NO_FLOWSHOP_H
#define GRAPHWIDGET_NO_FLOWSHOP_H

#include "basic_graphwidget.hpp"
#include "dot_parser.hpp"

#include <delayGraph/delayGraph.h>
#include <delayGraph/edge.h>

#include <QWidget>

namespace FlowShopVis {
class Node;

class GraphWidgetNoFlowshop : public BasicGraphWidget {
    Q_OBJECT
public:
    explicit GraphWidgetNoFlowshop(const DelayGraph::delayGraph &dg,
                                   const DOT::ColouredEdges &highlighted = {},
                                   QWidget *parent = nullptr);
};
} // namespace FlowShopVis
#endif // GRAPHWIDGET_NO_FLOWSHOP_H
