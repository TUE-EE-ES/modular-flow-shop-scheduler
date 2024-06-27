#include "production_line_bounds_widget.hpp"

#include "generic_widgets/expandable_scroll_area.hpp"
#include "module_bounds_widget.hpp"
#include "utils/layouts.hpp"

#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QStyle>

using namespace FlowShopVis;

FlowShopVis::ProductionLineBoundsWidget::ProductionLineBoundsWidget(QWidget *parent) :
    QWidget(parent) {
    setFocusPolicy(Qt::FocusPolicy::StrongFocus);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
    m_iterationLabel = new QLabel(this);
}

void ProductionLineBoundsWidget::setBounds(std::vector<FORPFSSPSD::GlobalBounds> bounds) {

    const auto mIdsView = bounds | std::views::join | std::views::keys;
    std::unordered_set<FORPFSSPSD::ModuleId> moduleIdsSet(mIdsView.begin(), mIdsView.end());
    std::vector<FORPFSSPSD::ModuleId> moduleIds(moduleIdsSet.begin(), moduleIdsSet.end());
    std::ranges::sort(moduleIds);

    std::unordered_map<FORPFSSPSD::ModuleId, std::vector<FORPFSSPSD::ModuleBounds>> moduleBounds;
    for (auto &bound : bounds) {
        for (const auto &moduleId : moduleIds) {
            auto it = bound.find(moduleId);
            if (it == bound.end()) {
                moduleBounds[moduleId].emplace_back();
            } else {
                moduleBounds[moduleId].emplace_back(std::move(it->second));
            }
        }
    }

    m_iteration = 0;
    m_maxIterations = bounds.size();
    QLayout *const layout = new QVBoxLayout();

    auto *const headerLayout = new QHBoxLayout();
    layout->addItem(headerLayout);

    headerLayout->addWidget(m_iterationLabel);
    updateLabel();

    headerLayout->addStretch();

    auto *const buttonPrevious =
            new QPushButton(QIcon(":/fonts/font-awesome/solid/chevron-left.svg"), "", this);
    connect(buttonPrevious, SIGNAL(clicked()), this, SLOT(previousIteration()));
    headerLayout->addWidget(buttonPrevious);

    auto *const buttonNext =
            new QPushButton(QIcon(":/fonts/font-awesome/solid/chevron-right.svg"), "", this);
    connect(buttonNext, SIGNAL(clicked()), this, SLOT(nextIteration()));
    headerLayout->addWidget(buttonNext);

    constexpr auto margin = 10;
    headerLayout->setContentsMargins(margin, 0, margin, 0);

    auto *const scroll = new ExpandableScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setSizeAdjustPolicy(QAbstractScrollArea::SizeAdjustPolicy::AdjustToContents);
    layout->addWidget(scroll);

    auto *const scrollWidget = new QWidget(scroll);
    scroll->setWidget(scrollWidget);

    auto *const scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->setSizeConstraint(QLayout::SizeConstraint::SetMinAndMaxSize);

    for (auto &module : moduleIds) {
        auto *moduleBoundsWidget = new ModuleBoundsWidget(module, scrollWidget);
        moduleBoundsWidget->setBounds(moduleBounds.at(module));

        connect(this,
                SIGNAL(iterationChanged(std::size_t)),
                moduleBoundsWidget,
                SLOT(iterationChanged(std::size_t)));

        scrollLayout->addWidget(moduleBoundsWidget);
    }
    scrollLayout->addStretch();

    layout->setContentsMargins(0, 0, 0, 0);

    auto *const oldLayout = this->layout();
    Utils::clearLayout(oldLayout);

    setLayout(layout);
}

void ProductionLineBoundsWidget::previousIteration() {
    if (m_iteration <= 0) {
        m_iteration = m_maxIterations - 1;
    } else {
        --m_iteration;
    }
    updateLabel();
    emit iterationChanged(m_iteration);
}

void ProductionLineBoundsWidget::nextIteration() {
    ++m_iteration;
    if (m_iteration >= m_maxIterations) {
        m_iteration = 0;
    }
    updateLabel();
    emit iterationChanged(m_iteration);
}

void ProductionLineBoundsWidget::updateLabel() {
    m_iterationLabel->setText((tr("Iteration %1 of %2")).arg(m_iteration + 1).arg(m_maxIterations));
}

void ProductionLineBoundsWidget::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Left) {
        previousIteration();
    } else if (event->key() == Qt::Key_Right) {
        nextIteration();
    }
}

