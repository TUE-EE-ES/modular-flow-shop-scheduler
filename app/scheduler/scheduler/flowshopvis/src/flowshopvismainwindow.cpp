#include "flowshopvismainwindow.hpp"
#include "ui_flowshopvismainwindow.h"

#include "flowshopwidget.hpp"
#include "graph/dot_parser.hpp"
#include "production_line/production_line_widget.hpp"

#include <fms/cg/builder.hpp>
#include <fms/problem/xml_parser.hpp>
#include <fms/solvers/dd.hpp>

#include <QFileDialog>
#include <QMessageBox>
#include <fstream>
#include <iostream>
#include <vector>

using namespace FlowShopVis;
QString openedFile;

FlowshopVisMainWindow::FlowshopVisMainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::FlowshopVisMainWindow) {

    ui->setupUi(this);
}

FlowshopVisMainWindow::~FlowshopVisMainWindow() { delete ui; }

void FlowshopVisMainWindow::openFlowShop(const QString &fileName,
                                         const bool &showPartialPaths,
                                         const auto &nodeId) {
    if (fileName.isEmpty() && showPartialPaths) {
        throw FmsSchedulerException(
                "Cannot open the constraint graph belonging to a node of the decision diagram.");
    }

    fms::problem::FORPFSSPSDXmlParser parser(fileName.toStdString());
    openedFile = fileName;

    QWidget *w;

    switch (parser.getFileType()) {
    case fms::problem::FORPFSSPSDXmlParser::FileType::MODULAR: {
        auto productionLine = parser.createProductionLine();

        // Initialize all the graphs
        for (auto &[moduleId, module] : productionLine.modules()) {
            auto dg = fms::cg::Builder::build(module);

            module.updateDelayGraph(std::move(dg));
        }
        w = new ProductionLineWidget(productionLine, this);

        break;
    }
    case fms::problem::FORPFSSPSDXmlParser::FileType::SHOP:
        w = new FlowshopWidget(parser.createFlowShop(), this, openedFile);
        if (showPartialPaths) {
            auto *flowshopWidget = dynamic_cast<FlowshopWidget *>(w);
            if (flowshopWidget) {
                flowshopWidget->openPartialSequence(nodeId);
            } else {
                qWarning() << "Error: QWidget pointer could not be cast to FlowshopWidget";
            }
        }
        break;
    default:
        throw FmsSchedulerException("Unknown file type");
    }

    connect(w,
            SIGNAL(showOperation(problem::ModuleId, problem::operation)),
            this,
            SLOT(showOperation(problem::ModuleId, problem::operation)));

    const QFileInfo fileInfo(fileName);
    QString extension = fileInfo.completeSuffix();

    QString tabName = fileInfo.completeBaseName();
    if (showPartialPaths) {
        tabName += "_" + QString::number(nodeId);
    }

    if (!extension.isEmpty()) {
        tabName += "." + extension;
    }
    const auto tabIndex = ui->tabWidget->addTab(w, tabName);
    ui->tabWidget->setTabToolTip(tabIndex, fileInfo.absoluteFilePath());
    ui->tabWidget->setCurrentIndex(tabIndex);
    on_tabWidget_tabBarClicked(tabIndex);
}

struct numeric_only : std::ctype<char> {
    numeric_only() : std::ctype<char>(get_table()) {}

    static std::ctype_base::mask const *get_table() {
        static std::vector<std::ctype_base::mask> rc(std::ctype<char>::table_size,
                                                     std::ctype_base::space);

        std::fill(&rc['0'], &rc[':'], std::ctype_base::digit);
        std::fill(&rc['-'], &rc['-'] + 1, std::ctype_base::digit);
        return &rc[0];
    }
};

void FlowshopVisMainWindow::openDotGraph(const QString &fileName) {
    if (fileName == QString()) {
        return;
    }

    auto [graph, colouredEdges, error] = DOT::parseDotFile(fileName.toStdString());

    if (!error.empty()) {
        QMessageBox::critical(nullptr, "Error", error.c_str());
        return;
    }

    auto *w = new ConstraintGraphWidget(std::move(graph), std::move(colouredEdges));
    connect(w, &ConstraintGraphWidget::showOperation, this, &FlowshopVisMainWindow::showOperation);
    ui->tabWidget->setCurrentIndex(ui->tabWidget->addTab(w, QFileInfo(fileName).fileName()));
}

void FlowshopVisMainWindow::on_tabWidget_tabCloseRequested(int index) {
    ui->tabWidget->removeTab(index);

    if (ui->tabWidget->count() == 0) {
        ui->actionOpenBounds->setEnabled(false);
        return;
    }

    on_tabWidget_tabBarClicked(ui->tabWidget->currentIndex());
}

void FlowshopVisMainWindow::on_tabWidget_tabBarClicked(int index) {
    auto *const w = ui->tabWidget->widget(index);
    if (w == nullptr) {
        return;
    }

    auto *const plw = dynamic_cast<ProductionLineWidget *>(w);
    if (plw != nullptr) {
        ui->actionOpenBounds->setEnabled(true);
    } else {
        ui->actionOpenBounds->setEnabled(false);
    }
}

void FlowshopVisMainWindow::on_actionOpen_triggered() {
    const QString filterSupported = tr("Supported files (*.xml, *.dot)");
    const QString filterFlowShop = tr("Flowshop definitions (*.xml)");
    const QString filterDotGraph = tr("Dot Graphs (*.dot)");
    QStringList filters;
    filters << filterSupported << filterFlowShop << filterDotGraph;

    QString selectedFilter;
    QString fileName = QFileDialog::getOpenFileName(
            this, tr("Open Flowshop or Graph"), "", filters.join(";;"), &selectedFilter);

    if (fileName == QString()) {
        return;
    }

    const auto fileExtension = QFileInfo(fileName).suffix();

    try {
        if (fileExtension == "xml") {
            openFlowShop(fileName, false, 0);
        } else if (fileExtension == "dot") {
            openDotGraph(fileName);
        } else {
            QMessageBox::critical(nullptr, "Error", "Unknown file type");
        }
    } catch (const FmsSchedulerException &e) {
        QMessageBox::critical(
                nullptr, "Exception while loading file", e.what() + QString("\n") + fileName);
    }
}

void FlowshopVisMainWindow::on_actionOpenBounds_triggered() {
    auto *const w = ui->tabWidget->currentWidget();
    auto *const plw = dynamic_cast<ProductionLineWidget *>(w);
    if (plw == nullptr) {
        return;
    }

    plw->open_bounds_clicked();
}

void FlowshopVisMainWindow::receiveNodeData(QString &msg) {
    try {
        openFlowShop(openedFile, true, msg.toInt());
    } catch (const FmsSchedulerException &e) {
        QMessageBox::critical(nullptr, "Exception while opening graph", e.what());
    }
}
