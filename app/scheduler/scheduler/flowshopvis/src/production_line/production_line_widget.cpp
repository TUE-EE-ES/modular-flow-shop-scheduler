#include "production_line_widget.hpp"

#include "production_line_graph_widget.hpp"

#include <fms/cg/builder.hpp>
#include <fms/solvers/sequence.hpp>

#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace FlowShopVis;

ProductionLineWidget::ProductionLineWidget(fms::problem::ProductionLine productionLine,
                                           QWidget *parent) :
    QWidget(parent), m_productionLine(std::move(productionLine)) {
    auto *const layoutMain = new QVBoxLayout(this);

    auto *const splitter = new QSplitter(this);
    layout()->addWidget(splitter);

    // Initialize the graph of all the instances of the production line
    for (auto &[moduleId, module] : m_productionLine.modules()) {
        auto dg = fms::cg::Builder::build(module);
        module.updateDelayGraph(dg);
    }

    m_graphWidget = new ProductionLineGraphWidget(m_productionLine, this);
    m_graphWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
    connect(m_graphWidget,
            &ProductionLineGraphWidget::showOperation,
            this,
            &ProductionLineWidget::showOperation);
    splitter->addWidget(m_graphWidget);

    m_boundsWidget = new ProductionLineBoundsWidget(this);
    m_boundsWidget->hide();
    connect(m_boundsWidget,
            SIGNAL(iterationChanged(std::size_t)),
            m_graphWidget,
            SLOT(sequenceSelected(std::size_t)));
    splitter->addWidget(m_boundsWidget);
}

void ProductionLineWidget::open_bounds_clicked() {
    QString fileName = QFileDialog::getOpenFileName(
            this,
            tr("Open bounds file"),
            "",
            tr("JSON with bounds (*.json);;CBOR with bounds (*.cbor)"));

    if (fileName == QString()) {
        return;
    }

    nlohmann::json jsonData;

    if (fileName.endsWith(".cbor")) {
        jsonData = nlohmann::json::from_cbor(fileName.toStdString());
    } else if (fileName.endsWith(".json")) {
        std::ifstream file(fileName.toStdString());
        jsonData = nlohmann::json::parse(file);
    } else {
        QMessageBox::critical(this, "Error", tr("Invalid file %1").arg(fileName));
        return;
    }

    if (!jsonData.contains("productionLine")) {
        QMessageBox::critical(this, "Error", tr("File doesn't contain \"productionLine\" key"));
        return;
    }

    auto jsonProductionLine = jsonData["productionLine"];
    if (!jsonProductionLine.contains("bounds")) {
        QMessageBox::critical(
                this, "Error", tr(R"("productionLine" object doesn't contain "bounds" key)"));
        return;
    }

    auto jsonBounds = jsonProductionLine["bounds"];
    auto bounds = fms::problem::allGlobalBoundsFromJSON(jsonBounds);

    m_boundsWidget->setBounds(std::move(bounds));
    m_boundsWidget->show();

    // Check if we also have a saved sequence
    if (!jsonProductionLine.contains("sequences")) {
        return;
    }

    auto jsonSequences = jsonProductionLine["sequences"];
    std::vector<fms::solvers::ProductionLineEdges> sequences;
    for (const auto &jsonSequence : jsonSequences) {
        auto sequence =
                fms::solvers::sequence::loadProductionLineEdges(jsonSequence, m_productionLine);
        sequences.push_back(std::move(sequence));
    }

    m_graphWidget->setSequencesHistory(m_productionLine, sequences);
}
