#include "window.hpp"

#include "graph_widget.hpp"

#include <fms/solvers/dd.hpp>

using namespace FlowShopVis::DD;

Window::Window(QWidget *parent) : QWidget(parent) {
    setWindowFlag(Qt::Window);
    setWindowTitle("DD Graph");

    auto *verticalLayout = new QVBoxLayout(this);

    m_graphWidget = new GraphWidget(this);
    verticalLayout->addWidget(m_graphWidget);

    setLayout(verticalLayout);
}

void Window::setData(std::shared_ptr<fms::solvers::dd::DDSolverData> data,
                     std::shared_ptr<fms::problem::Instance> instance) {
    m_data = data;
    m_instance = std::move(instance);
    m_graphWidget->setDDData(std::move(data));
}
