#ifndef FLOWSHOPVIS_DD_DD_WINDOW_HPP
#define FLOWSHOPVIS_DD_DD_WINDOW_HPP

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

namespace fms::solvers::dd {
struct DDSolverData;
} // namespace algorithms::DDSolver

namespace fms::problem {
class Instance;
} // namespace fms::problem

namespace FlowShopVis::DD {

class GraphWidget;

class Window : public QWidget {
    Q_OBJECT

public:
    explicit Window(QWidget *parent = nullptr);

    ~Window() override = default;

    void setData(std::shared_ptr<fms::solvers::dd::DDSolverData> data,
                 std::shared_ptr<fms::problem::Instance> instance);

private:
    GraphWidget *m_graphWidget;

    std::shared_ptr<fms::solvers::dd::DDSolverData> m_data;
    std::shared_ptr<fms::problem::Instance> m_instance;
};

} // namespace FlowShopVis::DD

#endif // FLOWSHOPVIS_DD_DD_WINDOW_HPP