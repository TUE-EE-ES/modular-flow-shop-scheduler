#ifndef FLOWSHOPVISMAINWINDOW_H
#define FLOWSHOPVISMAINWINDOW_H

#include <QMainWindow>
#include <QStatusBar>

#include <FORPFSSPSD/indices.hpp>
#include <FORPFSSPSD/operation.h>

namespace Ui {
class FlowshopVisMainWindow;
}

class FlowshopVisMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit FlowshopVisMainWindow(QWidget *parent = nullptr);
    ~FlowshopVisMainWindow();

    void openFlowShop(const QString & fileName);

    void openGraphWithoutFlowshop(const QString &fileName);

    void openDotGraph(const QString &fileName);
private slots:
    void showOperation(FORPFSSPSD::ModuleId moduleId, FORPFSSPSD::operation operation) {
        statusBar()->showMessage(QString::fromStdString(
                fmt::format("Module: {}, Operation: {}", moduleId, operation)));
    }

    void on_tabWidget_tabCloseRequested(int index);

    void on_tabWidget_tabBarClicked(int index);

    void on_actionOpen_triggered();

    void on_actionOpenBounds_triggered();

private:
    Ui::FlowshopVisMainWindow *ui;
};

#endif // FLOWSHOPVISMAINWINDOW_H
