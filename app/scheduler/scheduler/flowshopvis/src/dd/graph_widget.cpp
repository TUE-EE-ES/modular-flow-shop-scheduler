#include "graph_widget.hpp"

#include "graph/edge.hpp"

#include <QCoreApplication>
#include <QMessageBox>
#include <stack>

using namespace FlowShopVis::DD;
using namespace fms::dd;

constexpr const qreal kNodeRadius = 15.0;
constexpr const qreal kNodeSize = 2 * kNodeRadius;
constexpr const qreal kNodeHorizontalPadding = 75;
constexpr const qreal kNodeHorizontalSpacing = 2 * kNodeHorizontalPadding + kNodeSize;
constexpr const qreal kNodeVerticalPadding = 100;

GraphWidget::GraphWidget(QWidget *parent) : BasicGraphWidget(parent) {}

void GraphWidget::setDDData(std::shared_ptr<fms::solvers::dd::DDSolverData> data) {
    this->data = std::move(data);
    clear();

    if (!this->data) {
        return;
    }

    m_positionsWorker = new PositionsWorker(this);
    m_positionsWorker->setData(this->data);
    m_positionsWorker->setNodeXSpace(kNodeHorizontalSpacing);
    m_positionsWorker->setNodeYSpace(kNodeVerticalPadding);
    connect(m_positionsWorker,
            &PositionsWorker::positionsCalculated,
            this,
            &GraphWidget::positionsCalculated);
    connect(m_positionsWorker, &PositionsWorker::error, this, &GraphWidget::error);

    m_progressDialog = new QProgressDialog(
            "Calculating positions...", {}, 0, 2 * this->data->allStates.size(), this);
    m_progressDialog->setWindowModality(Qt::WindowModal);
    connect(m_positionsWorker,
            &PositionsWorker::progress,
            m_progressDialog,
            &QProgressDialog::setValue);
    m_progressDialog->show();

    m_positionsWorker->start();
}

void GraphWidget::clear() {
    BasicGraphWidget::clear();

    m_edges.clear();
    m_rootId = std::nullopt;
}

Node *GraphWidget::addNode(VertexId id, QPointF pos, QRectF &boundingBox) {
    auto *const node = new Node(QString::number(id));
    node->setPos(pos);
    boundingBox |= node->boundingRect().translated(node->pos());
    scene()->addItem(node);
    return node;
}

void GraphWidget::positionsCalculated() {
    if (m_positionsWorker == nullptr) {
        return;
    }

    auto positions = std::move(m_positionsWorker->positions());
    m_edges = std::move(m_positionsWorker->edges());
    m_rootId = m_positionsWorker->rootId();

    m_positionsWorker->deleteLater();
    m_positionsWorker = nullptr;

    m_progressDialog->setLabelText("Adding nodes to graph");
    m_progressDialog->setValue(0);
    m_progressDialog->setMaximum(positions.size());
    QCoreApplication::processEvents();

    QRectF boundingBox;
    std::vector<Node *> nodes(positions.size(), nullptr);
    std::size_t i = 0;
    for (const auto &[id, pos] : positions) {
        auto *const node = addNode(id, pos, boundingBox);
        nodes.at(id) = node;

        if (i % 10 == 0) {
            m_progressDialog->setValue(i + 1);
            QCoreApplication::processEvents();
        }
        ++i;
    }

    std::size_t totalEdges = 0;
    for (const auto &edges : m_edges) {
        totalEdges += edges.size();
    }

    m_progressDialog->setLabelText("Adding edges to graph");
    m_progressDialog->setMaximum(totalEdges);

    i = 0;
    for (std::size_t vFrom = 0; vFrom < m_edges.size(); ++vFrom) {
        auto *const nodeFrom = nodes.at(vFrom);
        for (const auto &vTo : m_edges.at(vFrom)) {
            auto *const nodeTo = nodes.at(vTo);

            const auto stateTo = data->allStates.at(vTo);
            const auto dgVId = stateTo->getLastScheduledOperation();

            QString edgeLabel;
            if (dgVId.has_value() && data->dg.isVisible(*dgVId)) {
                const auto op = data->dg.getOperation(dgVId.value());
                edgeLabel = QString::fromStdString(fmt::to_string(op));
            }

            auto *edge = new FlowShopVis::Edge(nodeFrom, nodeTo, edgeLabel);
            BasicGraphWidget::addEdge(edge, boundingBox);

            if (i % 10 == 0) {
                m_progressDialog->setValue(i + 1);
                QCoreApplication::processEvents();
            }
            ++i;
        }
    }

    setSceneRect(adjustMargin(boundingBox));

    auto *const rootNode = nodes.at(m_rootId.value());
    centerOn(rootNode);

    m_progressDialog->deleteLater();
    m_progressDialog = nullptr;
}

void GraphWidget::error(const QString &message) { QMessageBox::critical(this, "Error", message); }
