#include "activitywidget.hpp"

#include "graph/flow_shop_graph_widget.hpp"

#include <fms/algorithms/longest_path.hpp>

#include <QFile>
#include <QMessageBox>
#include <QTextStream>
#include <QWheelEvent>

using namespace FlowShopVis;

static constexpr double kRectScale = 2000;
static constexpr double kRectHeight = 50;
static constexpr double kRectSep = 10;
static constexpr double kRectDistance = kRectHeight + kRectSep;
static constexpr qreal kExtraPadding = 10;

static constexpr qreal kDefaultScale = 0.8;
static constexpr qreal kMinScale = 0.02;
static constexpr qreal kMaxScale = 100;

ActivityWidget::ActivityWidget(FlowShopGraphWidget &graphwidget, QWidget *parent) :
    QGraphicsView(parent), graphwidget(graphwidget) {
    auto *scene = new QGraphicsScene(this);
    scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    scene->setSceneRect(-200, -200, 400, 400);
    setScene(scene);
    setCacheMode(CacheBackground);
    setViewportUpdateMode(BoundingRectViewportUpdate);
    setRenderHint(QPainter::Antialiasing);
    setTransformationAnchor(AnchorUnderMouse);
    scale(kDefaultScale, kDefaultScale);
    setMinimumSize(400, 200);
    setWindowTitle(tr("Elastic Nodes"));
}

void ActivityWidget::openSolution(const fms::solvers::PartialSolution &solution, const fms::problem::Instance &instance) {
    const auto &jobs = instance.getJobsOutput();
    const auto &operations = instance.getOperationsFlowVector();
    const auto &dg = instance.getDelayGraph();
    const auto &asapst = solution.getASAPST();

    const fms::delay maxDelay = std::ranges::max(asapst);

    scene()->clear();

    QRectF boundingbox;
    auto maxDelayD = static_cast<qreal>(maxDelay);

    for (const auto &v : dg.getVertices()) {
        fms::problem::Operation op = v.operation;
        if (!fms::cg::ConstraintGraph::isVisible(v)) {
            continue;
        }
        fms::delay val = asapst.at(v.id);
        std::size_t machine = instance.getMachineOrder(op);

        QRectF rect(kRectScale * static_cast<qreal>(val) / maxDelayD,
                    static_cast<qreal>(machine) * kRectDistance,
                    kRectScale * static_cast<qreal>(instance.processingTimes(op)) / maxDelayD,
                    kRectHeight);
        auto *rectItem = scene()->addRect(rect);

        QColor color(FlowShopGraphWidget::getColor(machine));
        if (instance.getReEntrancies(op.jobId) == fms::problem::plexity::DUPLEX) {
            color = color.darker();
        }
        rectItem->setBrush(color);

        boundingbox = boundingbox.united(rect);
    }

    scene()->setSceneRect(
            boundingbox.adjusted(-kExtraPadding, -kExtraPadding, kExtraPadding, kExtraPadding));
}

void ActivityWidget::scaleView(qreal scaleFactor) {
    qreal factor = transform().scale(scaleFactor, 1).mapRect(QRectF(0, 0, 1, 1)).width();
    if (factor < kMinScale || factor > kMaxScale) {
        return;
    }

    scale(scaleFactor, 1);
}

#ifndef QT_NO_WHEELEVENT
void ActivityWidget::wheelEvent(QWheelEvent *event) {
    scaleView(std::pow(static_cast<double>(2), -event->angleDelta().y() / 240.0));
}
#endif
