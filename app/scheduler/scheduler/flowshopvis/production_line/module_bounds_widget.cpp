#include "module_bounds_widget.hpp"

#include <QLabel>
#include <QLayout>
#include <QTableView>

FlowShopVis::ModuleBoundsWidget::ModuleBoundsWidget(FORPFSSPSD::ModuleId moduleId,
                                                    QWidget *parent) :
    QWidget(parent), m_moduleId(moduleId), m_model(new BoundsModel(this)) {
    auto *const layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Module " + QString::number(moduleId.value)));

    auto *const layoutTable = new QHBoxLayout();
    layout->addItem(layoutTable);

    auto *tableView = new QTableView(this);
    tableView->setModel(m_model);
    tableView->setSizeAdjustPolicy(QAbstractScrollArea::SizeAdjustPolicy::AdjustToContents);
    tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    tableView->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    layoutTable->addWidget(tableView);
    layoutTable->addStretch();

    tableView->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
}
