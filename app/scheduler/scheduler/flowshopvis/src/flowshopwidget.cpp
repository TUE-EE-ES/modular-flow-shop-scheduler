#include "flowshopwidget.hpp"

#include "activitywidget.hpp"
#include "dd/window.hpp"
#include "graph/flow_shop_graph_widget.hpp"
#include "solver_worker.hpp"

#include <fms/cg/builder.hpp>
#include <fms/solvers/sequence.hpp>

#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include <fmt/chrono.h>

using namespace FlowShopVis;

// NOLINTBEGIN(cppcoreguidelines-owning-memory): Qt has a memory management system

FlowshopWidget::FlowshopWidget(fms::problem::Instance instance, QWidget *parent, QString fileName) :
    QWidget(parent),
    m_instance(std::make_shared<fms::problem::Instance>(std::move(instance))),
    m_fileName(std::move(fileName)) {

    auto *verticalLayout = new QVBoxLayout(this);
    verticalLayout->setSpacing(6);
    verticalLayout->setContentsMargins(11, 11, 11, 11);
    verticalLayout->setObjectName(QStringLiteral("verticalLayout"));

    auto *buttonsLayout = new QHBoxLayout();
    verticalLayout->addLayout(buttonsLayout);

    auto *generateDD = new QPushButton("Run scheduler", this);
    buttonsLayout->addWidget(generateDD);
    connect(generateDD, SIGNAL(clicked(bool)), this, SLOT(runSchedulerClicked()));

    auto *openResults = new QPushButton("Open results file", this);
    buttonsLayout->addWidget(openResults);
    connect(openResults, SIGNAL(clicked(bool)), this, SLOT(openResultsClicked()));

    buttonsLayout->addStretch();

    if (!m_instance->isGraphInitialized()) {
        m_instance->updateDelayGraph(fms::cg::Builder::build(*m_instance));
    }

    graphWidget = new FlowShopGraphWidget(*m_instance, this);
    connect(graphWidget, &FlowShopGraphWidget::showOperation, this, &FlowshopWidget::showOperation);
    verticalLayout->addWidget(graphWidget);

    activityWidget = new ActivityWidget(*graphWidget, this);
    verticalLayout->addWidget(activityWidget);
}

void FlowshopWidget::openResultsClicked() {
    try {
        const QString fileJSON = tr("JSON FMS data file (*.fms.json)");
        const QString fileCBOR = tr("CBOR FMS data file (*.fms.cbor)");

        QStringList filters;
        filters << fileJSON << fileCBOR;

        const QString fileName = QFileDialog::getOpenFileName(
                this, tr("Open sequence file"), "", filters.join(";;"));
        if (fileName.isEmpty()) {
            return;
        }

        const fms::cli::CLIArgs args{.sequenceFile = fileName.toStdString()};
        auto [solutions, _] = fms::solvers::sequence::solve(*m_instance, args);
        const auto &solution = solutions.front();

        graphWidget->setSequences(solution.getChosenSequencesPerMachine(), *m_instance);
        activityWidget->openSolution(solution, *m_instance);

    } catch (const FmsSchedulerException &e) {
        QMessageBox::critical(nullptr, "Unable to open sequence file", e.what());
    }
}

void FlowshopWidget::openPartialSequence(const fms::dd::VertexId nodeId) {
    const auto ddData = getDDData();
    if (!ddData) {
        QMessageBox::critical(nullptr, "Error", "Decision diagram data not found");
        return;
    }

    const auto vertex = ddData->states.at(nodeId);

    try {
        graphWidget->setSequences(vertex->getMachinesSequences(), *m_instance);
    } catch (FmsSchedulerException &e) {
        QMessageBox::critical(nullptr, "Exception while loading constraint graph", e.what());
    }
}

// Add this method to the FlowshopWidget class to handle results
void FlowshopWidget::handleResults() {
    // Remove the progress dialog
    m_progressDialog->deleteLater();
    m_progressDialog = nullptr;

    m_solverData = m_worker->getData();
    if (m_solverData == nullptr) {
        QMessageBox::critical(nullptr, "Error", "Solver data not found");
        return;
    }

    m_worker->deleteLater();
    m_worker = nullptr;

    auto ddDataPtr = getDDData();
    if (!ddDataPtr) {
        QMessageBox::critical(nullptr, "Error", "Decision diagram data not found");
        return;
    }

    m_ddWindow = new FlowShopVis::DD::Window(this);
    m_ddWindow->setData(std::move(ddDataPtr), m_instance);
    m_ddWindow->show();
}

// Add this method to the FlowshopWidget class to handle errors
void FlowshopWidget::handleError(const QString &error) {
    QMessageBox::critical(nullptr, "Unable to generate JSON. Error: ", error);

    // Hide the progress dialog
    m_progressDialog->hide();
}

void FlowshopWidget::runSchedulerClicked() {
    if (m_fileName.isEmpty()) {
        throw FmsSchedulerException("No file opened");
    }

    // Solver worker thread
    m_worker = new SolverWorker(this);
    connect(m_worker, &SolverWorker::resultReady, this, &FlowshopWidget::handleResults);
    connect(m_worker, &SolverWorker::errorOccurred, this, &FlowshopWidget::handleError);
    connect(m_worker, &SolverWorker::iteration, this, &FlowshopWidget::iteration);
    connect(m_worker, &SolverWorker::parsing, this, &FlowshopWidget::parsing);
    m_worker->setArgs(fms::cli::CLIArgs{
            .inputFile = m_fileName.toStdString(),
            .outputFile = "schedule",
            .maxIterations = 10000,
            .algorithm = fms::cli::AlgorithmType::DD,
            .algorithmOptions = {fms::solvers::dd::kStoreHistory},
            .outputFormat = fms::cli::ScheduleOutputFormat::JSON,
    });

    m_startTime = std::chrono::high_resolution_clock::now();

    m_progressDialog = new QProgressDialog("Generating decision diagram...", {}, 0, 0, this);

    m_progressDialog->setWindowModality(Qt::WindowModal);
    m_progressDialog->setMinimumDuration(0);
    m_progressDialog->show();

    // Start the computation
    m_worker->start();
}

void FlowshopWidget::iteration(std::size_t iteration) {
    if (m_progressDialog == nullptr) {
        return;
    }

    auto diff = std::chrono::high_resolution_clock::now() - m_startTime;

    m_progressDialog->setLabelText(
            fmt::format(
                    "Generating decision diagram...\nIteration: {}\nTime: {:%S}", iteration, diff)
                    .c_str());
}

void FlowshopWidget::parsing(int progress) {
    if (m_progressDialog == nullptr) {
        return;
    }

    m_progressDialog->setLabelText(QString("Parsing input file results"));

    m_progressDialog->setMinimum(0);
    m_progressDialog->setMaximum(100);
    m_progressDialog->setValue(progress);
}

std::shared_ptr<fms::solvers::dd::DDSolverData> FlowshopWidget::getDDData() {
    return std::dynamic_pointer_cast<fms::solvers::dd::DDSolverData>(m_solverData);
}

// NOLINTEND(cppcoreguidelines-owning-memory)
