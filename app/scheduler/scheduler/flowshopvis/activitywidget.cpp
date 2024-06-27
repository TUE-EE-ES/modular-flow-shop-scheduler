#include "activitywidget.hpp"

#include "graph/graphwidget.hpp"

#include <longest_path.h>

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

ActivityWidget::ActivityWidget(GraphWidget &graphwidget, QWidget *parent) :
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

void ActivityWidget::open_asapst_file(const QString &fileName,
                                      const FORPFSSPSD::Instance &instance) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::information(nullptr, "Error reading file", file.errorString());
    }
    QTextStream in(&file);

    std::size_t jobIndex = 0;
    const auto &jobs = instance.getJobsOutput();
    const auto &operations = instance.getOperationsFlowVector();
    const auto &dg = instance.getDelayGraph();
    std::vector<delay> asapst = algorithm::LongestPath::initializeASAPST(instance.getDelayGraph());

    delay maxDelay = std::numeric_limits<delay>::min();

    while (!in.atEnd()) {
        const auto job = jobs[jobIndex];
        QString line = in.readLine();
        if (line.isEmpty()) {
            continue;
        }
        auto fields = line.split(",");
        std::size_t operationIndex = 0;
        for (auto &str : fields) {
            str = str.trimmed();
            auto operationId = operations[operationIndex++];
            if (str.isEmpty()) {
                continue;
            }
            if (!dg.has_vertex({job, operationId})) {
                QMessageBox::critical(
                        nullptr,
                        "Error reading file",
                        fmt::format("Vertex ({}, {})  does not exist, but has entry in ASAPST "
                                    "file; giving up reading ASAPST",
                                    job,
                                    operationId)
                                .c_str());
                return;
            }
            auto val = static_cast<delay>(str.toLongLong());
            asapst[dg.get_vertex({job, operationId}).id] = val;
            maxDelay = std::max(maxDelay, val);
        }

        jobIndex++;
    }
    scene()->clear();

    QRectF boundingbox;
    auto maxDelayD = static_cast<qreal>(maxDelay);

    for (const auto &v : dg.get_vertices()) {
        FORPFSSPSD::operation op = v.operation;
        if (!DelayGraph::delayGraph::is_visible(v)) {
            continue;
        }
        delay val = asapst.at(v.id);
        std::size_t machine = instance.getMachineOrder(op);

        QRectF rect(kRectScale * static_cast<qreal>(val) / maxDelayD,
                    static_cast<qreal>(machine) * kRectDistance,
                    kRectScale * static_cast<qreal>(instance.processingTimes(op)) / maxDelayD,
                    kRectHeight);
        auto *rectItem = scene()->addRect(rect);

        QColor color(GraphWidget::getColor(machine));
        if (instance.getPlexity(op.jobId) == FORPFSSPSD::Plexity::DUPLEX) {
            color = color.darker();
        }
        rectItem->setBrush(color);

        boundingbox = boundingbox.united(rect);
    }

    scene()->setSceneRect(
            boundingbox.adjusted(-kExtraPadding, -kExtraPadding, kExtraPadding, kExtraPadding));

    PartialSolution ps = instance.determine_partial_solution(asapst);

    graphwidget.setPartialSolution(ps, instance);

    file.close();
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
