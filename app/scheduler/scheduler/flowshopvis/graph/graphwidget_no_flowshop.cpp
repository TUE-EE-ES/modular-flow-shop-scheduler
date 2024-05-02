#include "graphwidget_no_flowshop.hpp"

#include "edge.hpp"
#include "node.hpp"

#include <QDebug>
#include <QKeyEvent>
#include <QOpenGLWidget>

using namespace FlowShopVis;
using namespace DelayGraph;

static constexpr qreal kNodeRadius = 15;

GraphWidgetNoFlowshop::GraphWidgetNoFlowshop(const delayGraph &dg,
                                             const DOT::ColouredEdges &highlighted,
                                             QWidget *parent) :
    BasicGraphWidget(parent) {

    QRectF boundingbox;
    const FORPFSSPSD::ModuleId moduleId{0};

    // add all vertices
    for (const DelayGraph::vertex &v : dg.get_vertices()) {
        if (!delayGraph::is_visible(v)) {
            continue;
        }

        const auto &op = v.operation;
        QPointF pos(100 * op.jobId, 200 * op.operationId);
        QColor color(getColor(v.operation.operationId));

        addNode(moduleId, op, pos, color, boundingbox);
    }

    addModuleEdges(moduleId, dg, boundingbox, highlighted);
    setSceneRect(boundingbox.adjusted(-500, -500, 500, 500));
}
