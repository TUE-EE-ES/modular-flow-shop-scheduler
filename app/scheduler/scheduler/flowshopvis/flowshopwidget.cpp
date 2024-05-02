#include "flowshopwidget.hpp"

#include "activitywidget.hpp"
#include "graph/graphwidget.hpp"
#include "delayGraph/builder.hpp"

#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QTextStream>
#include <QVBoxLayout>
#include <fstream>

using namespace FlowShopVis;

FlowshopWidget::FlowshopWidget(FORPFSSPSD::Instance instance, QWidget *parent) :
    QWidget(parent), m_instance(std::move(instance)) {

    auto *verticalLayout = new QVBoxLayout(this);
    verticalLayout->setSpacing(6);
    verticalLayout->setContentsMargins(11, 11, 11, 11);
    verticalLayout->setObjectName(QStringLiteral("verticalLayout"));

    auto *buttonsLayout = new QHBoxLayout();
    verticalLayout->addLayout(buttonsLayout);

    auto *open_asapst = new QPushButton("Open ASAPST", this);
    buttonsLayout->addWidget(open_asapst);

    connect(open_asapst, SIGNAL(clicked(bool)), this, SLOT(open_asapst_clicked()));

    auto *open_sequence = new QPushButton("Open sequence file", this);
    buttonsLayout->addWidget(open_sequence);
    buttonsLayout->addStretch();

    connect(open_sequence, SIGNAL(clicked(bool)), this, SLOT(open_sequence_clicked()));

    if (!m_instance.isGraphInitialized()) {
        m_instance.updateDelayGraph(DelayGraph::Builder::build(m_instance));
    }

    graphWidget = new GraphWidget(m_instance, this);
    connect(graphWidget,
            SIGNAL(showOperation(FORPFSSPSD::ModuleId, FORPFSSPSD::operation)),
            this,
            SIGNAL(showOperation(FORPFSSPSD::ModuleId, FORPFSSPSD::operation)));
    verticalLayout->addWidget(graphWidget);

    activityWidget = new ActivityWidget(*graphWidget, this);
    verticalLayout->addWidget(activityWidget);
}

void FlowshopWidget::open_asapst_clicked()
{
    try {
        QString fileName = QFileDialog::getOpenFileName(this,
            tr("Open ASAPST"), "", tr("ASAPST text file (*.txt)"));
        if(!fileName.isEmpty()) {
            activityWidget->open_asapst_file(fileName, m_instance);
        }
    } catch(const FmsSchedulerException &e) {
        QMessageBox::critical(nullptr, "Unable to open ASAPST file", e.what());
    }
}

void FlowshopWidget::open_sequence_clicked()
{
    try {
        QString fileName = QFileDialog::getOpenFileName(
                this, tr("Open sequence file"), "", tr("sequence text file (*.sequence)"));
        if(fileName != QString()) {
            std::ifstream file(fileName.toStdString());
            if(!file.is_open()) {
                throw FmsSchedulerException("Unable to load sequence file");
            }
            graphWidget->setPartialSolution(m_instance.loadSequence(file), m_instance);
            file.close();
        }

    } catch(const FmsSchedulerException &e) {
        QMessageBox::critical(nullptr, "Unable to open sequence file", e.what());
    }
}
