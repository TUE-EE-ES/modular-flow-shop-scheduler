#include "flow_shop_graph_widget.hpp"

#include "edge.hpp"
#include "operation_node.hpp"

#include <fms/problem/indices.hpp>

#include <QDebug>
#include <QKeyEvent>
#include <fmt/ostream.h>
#include <iostream>

using namespace FlowShopVis;

constexpr static const qreal kSpacing = 20;
constexpr static const float kEdgeBend = 30;
constexpr static const qreal kOpXPos = 100;
constexpr static const qreal kOpYPos = 200;

// NOLINTBEGIN(cppcoreguidelines-owning-memory): Qt has a memory management system

FlowShopGraphWidget::FlowShopGraphWidget(const fms::problem::Instance &instance,
                                         QWidget *parent,
                                         const fms::algorithms::paths::PathTimes &ASAPST,
                                         const fms::algorithms::paths::PathTimes &ALAPST) :
    ConstraintGraphWidget(parent) {

    QRectF boundingbox;
    const fms::problem::ModuleId moduleId{0};

    if (!instance.isGraphInitialized()) {
        // Throw exception
        throw std::runtime_error("Graph not initialized");
    }

    const fms::cg::ConstraintGraph &dg = instance.getDelayGraph();

    std::unordered_map<fms::problem::MachineId, std::size_t> machineToIndex;
    std::size_t machineIndex = 0;
    for (const auto &machine : instance.getMachines()) {
        machineToIndex[machine] = machineIndex++;
    }

    // add all vertices
    for (const fms::cg::Vertex &v : dg.getVertices()) {
        if (!fms::cg::ConstraintGraph::isVisible(v)) {
            continue;
        }
        const auto &op = v.operation;
        QPointF pos(kOpXPos * op.jobId.value, kOpYPos * op.operationId);
        QColor color(getColor(machineToIndex.at(instance.getMachine(op))));

        if (ASAPST.empty() || ALAPST.empty()) {
            addNode(moduleId, op, v.id, pos, color, boundingbox);
        } else {
            addNode(moduleId, op, v.id, pos, color, boundingbox, ASAPST.at(v.id), ALAPST.at(v.id));
        }
    }

    addModuleEdges(moduleId, dg, boundingbox);
    setSceneRect(adjustMargin(boundingbox, kSpacing));
}

void FlowShopGraphWidget::setPartialSolution(const fms::solvers::PartialSolution &ps,
                                             const fms::problem::Instance &instance) {
    const auto &dg = instance.getDelayGraph();
    const auto &nodes = getNodes().at(fms::problem::ModuleId{0});

    QRectF boundingbox = setPartialSequence(ps, instance);

    const auto sequence = ps.getInferredInputSequence(instance);
    const QPen pen(Qt::black, 3);

    auto indices = std::views::iota(0, static_cast<int>(sequence.size() - 1));
    auto zipped = indices | std::views::transform([&sequence](int i) {
                      return std::pair{sequence[i], sequence[i + 1]};
                  });
    for (const auto &[op, opNext] : zipped) {
        const auto srcNodeIt = nodes.find(op);
        const auto dstNodeIt = nodes.find(opNext);

        if (srcNodeIt == nodes.end()) {
            fmt::print(std::cerr, "ERR: no source for {}\n", op);
            continue;
        }

        if (dstNodeIt == nodes.end()) {
            fmt::print(std::cerr, "ERR: no destination for {}\n", opNext);
            continue;
        }

        float bend = 0;
        if (!instance.getDelayGraph().hasEdge(op, opNext)) {
            bend = kEdgeBend;
        }

        const auto weight = instance.query(op, opNext);
        addEdge(srcNodeIt->second, dstNodeIt->second, weight, bend, pen, boundingbox);
    }

    setSceneRect(adjustMargin(boundingbox, kSpacing));
}

QRectF FlowShopGraphWidget::setPartialSequence(const fms::solvers::PartialSolution &ps,
                                               const fms::problem::Instance &instance) {
    return setSequences(ps.getChosenSequencesPerMachine(), instance);
}

QRectF FlowShopGraphWidget::setSequences(const fms::solvers::MachinesSequences &machinesSequences,
                                         const fms::problem::Instance &instance) {
    // First remove the edges from the previous sequence
    for (auto *const edge : m_currentSequenceEdges) {
        auto *const nodeFrom = edge->sourceNode();
        auto *const nodeTo = edge->destNode();

        nodeFrom->removeEdge(edge);
        nodeTo->removeEdge(edge);
        scene()->removeItem(edge);
        delete edge;
    }
    m_currentSequenceEdges.clear();

    // Now we can add the new edges
    const auto &dg = instance.getDelayGraph();
    const auto &nodes = getNodes().at(fms::problem::ModuleId{0});

    QRectF boundingbox = adjustMargin(sceneRect(), kSpacing);
    const QPen pen(Qt::black, 3);

    for (const auto &[machineId, ops] : machinesSequences) {
        auto indices = std::views::iota(0, static_cast<int>(ops.size() - 1));
        auto zipped = indices | std::views::transform([&ops](int i) {
                          return std::pair{ops[i], ops[i + 1]};
                      });

        for (const auto &[op, opNext] : zipped) {
            const auto &vSrc = dg.getVertex(op);
            const auto &vDst = dg.getVertex(opNext);

            if (!fms::cg::ConstraintGraph::isVisible(vSrc)
                || !fms::cg::ConstraintGraph::isVisible(vDst)) {
                continue;
            }

            auto *const nodeFrom = nodes.at(vSrc.operation);
            auto *const nodeTo = nodes.at(vDst.operation);

            auto *edge = addEdge(nodeFrom, nodeTo, instance.query(op, opNext), 0, pen, boundingbox);
            m_currentSequenceEdges.push_back(edge);
        }
    }

    setSceneRect(adjustMargin(boundingbox, kSpacing));
    return boundingbox;
}

// NOLINTEND(cppcoreguidelines-owning-memory): Qt has a memory management system
