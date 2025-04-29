#ifndef FLOWSHOPWIDGET_H
#define FLOWSHOPWIDGET_H

#include <fms/dd/vertex.hpp>
#include <fms/problem/flow_shop.hpp>
#include <fms/solvers/dd.hpp>
#include <fms/solvers/solver_data.hpp>

#include <QProgressDialog>
#include <QWidget>

// NOLINTBEGIN(readability-redundant-access-specifiers)

namespace FlowShopVis {
class FlowShopGraphWidget;
class ActivityWidget;
class SolverWorker;

namespace DD {
class Window;
}
} // namespace FlowShopVis

class FlowshopWidget : public QWidget {
    Q_OBJECT
public:
    explicit FlowshopWidget(fms::problem::Instance instance,
                            QWidget *parent = nullptr,
                            QString fileName = nullptr);
    void openPartialSequence(fms::dd::VertexId nodeId);

signals:
    void showOperation(fms::problem::ModuleId, fms::problem::Operation);
public slots:
    void runSchedulerClicked();
    void openResultsClicked();
    void handleResults();
    void handleError(const QString &error);

private slots:
    void iteration(std::size_t iteration);
    void parsing(int progress);

private:
    std::shared_ptr<fms::solvers::dd::DDSolverData> getDDData();

    // Qt elements
    FlowShopVis::ActivityWidget *activityWidget;
    FlowShopVis::FlowShopGraphWidget *graphWidget;
    FlowShopVis::SolverWorker *m_worker = nullptr;
    QProgressDialog *m_progressDialog = nullptr;
    FlowShopVis::DD::Window *m_ddWindow = nullptr;

    std::shared_ptr<fms::solvers::SolverData> m_solverData;
    std::shared_ptr<fms::problem::Instance> m_instance;
    QString m_fileName;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;
    std::vector<fms::dd::VertexId> m_openedNodes;
};

// NOLINTEND(readability-redundant-access-specifiers)

#endif // FLOWSHOPWIDGET_H
