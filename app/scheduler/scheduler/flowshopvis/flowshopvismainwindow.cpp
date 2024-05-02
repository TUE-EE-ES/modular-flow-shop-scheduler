#include "flowshopvismainwindow.hpp"
#include "ui_flowshopvismainwindow.h"

#include "FORPFSSPSD/xmlParser.h"
#include "flowshopwidget.hpp"
#include "graph/dot_parser.hpp"
#include "graph/graphwidget_no_flowshop.hpp"
#include "production_line/production_line_widget.hpp"

#include <delayGraph/builder.hpp>

#include <QFileDialog>
#include <QMessageBox>
#include <iostream>

using namespace FlowShopVis;
using namespace DelayGraph;

FlowshopVisMainWindow::FlowshopVisMainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::FlowshopVisMainWindow) {
    ui->setupUi(this);
}

FlowshopVisMainWindow::~FlowshopVisMainWindow() { delete ui; }

void FlowshopVisMainWindow::openFlowShop(const QString &fileName) {
    FORPFSSPSDXmlParser parser(fileName.toStdString());
    QWidget *w;

    switch (parser.getFileType()) {
    case FORPFSSPSDXmlParser::FileType::MODULAR: {
        auto productionLine = parser.createProductionLine();

        // Initialize all the graphs
        for (auto &[moduleId, module] : productionLine.modules()) {
            auto dg = DelayGraph::Builder::build(module);
            module.updateDelayGraph(std::move(dg));
        }
        w = new ProductionLineWidget(productionLine, this);

        break;
    }
    case FORPFSSPSDXmlParser::FileType::SHOP:
        w = new FlowshopWidget(parser.createFlowShop());
        break;
    default:
        throw FmsSchedulerException("Unknown file type");
    }

    connect(w,
            SIGNAL(showOperation(FORPFSSPSD::ModuleId, FORPFSSPSD::operation)),
            this,
            SLOT(showOperation(FORPFSSPSD::ModuleId, FORPFSSPSD::operation)));

    const QFileInfo fileInfo(fileName);
    const auto tabIndex = ui->tabWidget->addTab(w, fileInfo.fileName());
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

void FlowshopVisMainWindow::openGraphWithoutFlowshop(const QString &fileName) {
    delayGraph graph;

    std::string line;

    if (fileName == QString()) {
        return;
    }

    std::ifstream file(fileName.toStdString());
    if (!file.is_open()) {
        throw FmsSchedulerException("Unable to load graph file");
    }

    while (std::getline(file, line)) {
        if (line.empty())
            continue; // skip empty lines
        std::istringstream line_stream(line);

        // read source and destination from line
        FORPFSSPSD::operation src, dst;
        if (!(line_stream >> src >> dst)) {
            throw FmsSchedulerException(
                    "Unable to read graph from file; operation syntax does not match");
        }

        if (!graph.has_vertex(src)) {
            graph.add_vertex(src);
        }
        if (!graph.has_vertex(dst)) {
            graph.add_vertex(dst);
        }

        // read the weight:
        line_stream.imbue(std::locale(std::locale(), new numeric_only()));
        delay weight;
        if (!(line_stream >> weight)) {
            throw FmsSchedulerException("Unable to read edge weight!");
        }

        graph.add_edge(graph.get_vertex(src), graph.get_vertex(dst), weight);
    }

    auto *w = new GraphWidgetNoFlowshop(graph);
    connect(w,
            SIGNAL(showOperation(FORPFSSPSD::ModuleId, FORPFSSPSD::operation)),
            this,
            SLOT(showOperation(FORPFSSPSD::ModuleId, FORPFSSPSD::operation)));
    ui->tabWidget->setCurrentIndex(ui->tabWidget->addTab(w, QFileInfo(fileName).fileName()));
}

void FlowshopVisMainWindow::openDotGraph(const QString &fileName) {
    if (fileName == QString()) {
        return;
    }

    auto [graph, colouredEdges, error] = DOT::parseDotFile(fileName.toStdString());

    if (!error.empty()) {
        QMessageBox::critical(nullptr, "Error", error.c_str());
        return;
    }

    auto *w = new GraphWidgetNoFlowshop(std::move(graph), std::move(colouredEdges));
    connect(w,
            SIGNAL(showOperation(FORPFSSPSD::ModuleId, FORPFSSPSD::operation)),
            this,
            SLOT(showOperation(FORPFSSPSD::ModuleId, FORPFSSPSD::operation)));
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
    const QString filterFlowShop = tr("Flowshop definitions (*.xml)");
    const QString filterDotGraph = tr("Dot Graphs (*.dot)");
    const QString filterGraph = tr("Graph definitions (*.txt)");
    QStringList filters;
    filters << filterFlowShop << filterDotGraph << filterGraph;

    QString selectedFilter;
    QString fileName = QFileDialog::getOpenFileName(
            this, tr("Open Flowshop or Graph"), "", filters.join(";;"), &selectedFilter);

    if (fileName == QString()) {
        return;
    }

    try {
        if (selectedFilter == filterFlowShop) {
            openFlowShop(fileName);
        } else if (selectedFilter == filterDotGraph) {
            openDotGraph(fileName);
        } else if (selectedFilter == filterGraph) {
            openGraphWithoutFlowshop(fileName);
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
