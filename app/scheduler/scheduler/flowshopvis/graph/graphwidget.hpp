#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include "basic_graphwidget.hpp"

#include <FORPFSSPSD/FORPFSSPSD.h>

#include <QString>
#include <QWidget>
#include <fmt/core.h>

namespace FlowShopVis {
class Node;

class GraphWidget : public BasicGraphWidget {
    Q_OBJECT
public:
    explicit GraphWidget(const FORPFSSPSD::Instance &instance, QWidget *parent = nullptr);

    void setPartialSolution(const PartialSolution &ps, const FORPFSSPSD::Instance &instance);
};
} // namespace FlowShopVis
#endif // GRAPHWIDGET_H
