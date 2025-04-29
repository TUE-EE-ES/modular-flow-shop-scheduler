#ifndef FLOWSHOPVISMAINWINDOW_H
#define FLOWSHOPVISMAINWINDOW_H

#include "flowshopwidget.hpp"

#include <fms/cg/constraint_graph.hpp>
#include <fms/problem/indices.hpp>
#include <fms/problem/operation.hpp>

#include <QMainWindow>
#include <QStatusBar>

namespace Ui {
class FlowshopVisMainWindow;
}

class FlowshopVisMainWindow : public QMainWindow {
    Q_OBJECT

public slots:
    void receiveNodeData(const QString &msg);

public:
    explicit FlowshopVisMainWindow(QWidget *parent = nullptr);
    ~FlowshopVisMainWindow() override;

    void openFlowShop(const QString &fileName,
                      const bool &showPartialPaths = false,
                      const auto &nodeId = 0);

    void openDotGraph(const QString &fileName);

private slots:
    void showOperation(fms::problem::ModuleId moduleId, fms::problem::Operation operation, fms::cg::VertexId vertexId) {
        statusBar()->showMessage(QString::fromStdString(
                fmt::format("Module: {}, Operation: {}, VId: {}", moduleId, operation, vertexId)));
    }

    void on_tabWidget_tabCloseRequested(int index);

    void on_tabWidget_tabBarClicked(int index);

    void on_actionOpen_triggered();

    void on_actionOpenBounds_triggered();

    void receiveNodeData(QString &msg);

private:
    Ui::FlowshopVisMainWindow *ui;
};

#endif // FLOWSHOPVISMAINWINDOW_H
