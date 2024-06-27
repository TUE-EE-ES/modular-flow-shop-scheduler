#include "graphwidget.hpp"

#include "FORPFSSPSD/indices.hpp"
#include "edge.hpp"
#include "node.hpp"

#include <QDebug>
#include <QKeyEvent>
#include <fmt/ostream.h>
#include <iostream>

// #define _USE_MATH_DEFINES
// #include <cmath>
// #include <utility>

using namespace FlowShopVis;
using namespace DelayGraph;

GraphWidget::GraphWidget(const FORPFSSPSD::Instance &instance, QWidget *parent) :
    BasicGraphWidget(parent) {

    QRectF boundingbox;
    const FORPFSSPSD::ModuleId moduleId{0};

    if (!instance.isGraphInitialized()) {
        // Throw exception
        throw std::runtime_error("Graph not initialized");
    }

    const delayGraph &dg = instance.getDelayGraph();

    std::unordered_map<FORPFSSPSD::MachineId, std::size_t> machineToIndex;
    std::size_t machineIndex = 0;
    for (const auto &machine : instance.getMachines()) {
        machineToIndex[machine] = machineIndex++;
    }

    // add all vertices
    for (const vertex &v : dg.get_vertices()) {
        if (!delayGraph::is_visible(v)) {
            continue;
        }
        const auto &op = v.operation;
        QPointF pos(100 * op.jobId.value, 200 * op.operationId);
        QColor color(getColor(machineToIndex.at(instance.getMachine(op))));

        addNode(moduleId, op, pos, color, boundingbox);
    }

    addModuleEdges(moduleId, dg, boundingbox);
    setSceneRect(boundingbox.adjusted(-20, -20, 20, 20));
}

void GraphWidget::setPartialSolution(const PartialSolution &ps,
                                     const FORPFSSPSD::Instance &instance) {
    const auto &dg = instance.getDelayGraph();
    const auto &nodes = getNodes().at(FORPFSSPSD::ModuleId{0});

    QRectF boundingbox = sceneRect().adjusted(20, 20, -20, -20);
    for (const auto &e : ps.getAllChosenEdges()) {
        const auto &vSrc = dg.get_vertex(e.src);
        const auto &vDst = dg.get_vertex(e.dst);

        if (!delayGraph::is_visible(vSrc) || !delayGraph::is_visible(vDst)) {
            continue;
        }

        Edge *edge = new Edge(nodes.at(vSrc.operation), nodes.at(vDst.operation), e.weight, 0);
        edge->setPen(QPen(Qt::black, 3));
        scene()->addItem(edge);
        boundingbox |= edge->boundingRect();
    }

    const auto edges = instance.infer_pim_edges(ps);
    auto *const sc = scene();
    for (const DelayGraph::edge &e : edges) {
        if (dg.is_source(e.src) || dg.is_source(e.dst)) {
            continue;
        }

        auto srcOp = dg.get_vertex(e.src).operation;
        auto dstOp = dg.get_vertex(e.dst).operation;

        const auto srcNodeIt = nodes.find(srcOp);
        const auto dstNodeIt = nodes.find(dstOp);

        if (srcNodeIt == nodes.end()) {
            fmt::print(std::cerr, "ERR: no source for {}\n", srcOp);
            continue;
        }

        if (dstNodeIt == nodes.end()) {
            fmt::print(std::cerr, "ERR: no destination for {}\n", dstOp);
            continue;
        }

        unsigned int bend = 0;
        if (!instance.getDelayGraph().has_edge(e.src, e.dst)) {
            bend = 30;
        }

        Edge *edge = new Edge(srcNodeIt->second, dstNodeIt->second, e.weight, bend);
        edge->setPen(QPen(Qt::black, 3));
        sc->addItem(edge);
        boundingbox |= edge->boundingRect();
    }

    setSceneRect(boundingbox.adjusted(-20, -20, 20, 20));
}
