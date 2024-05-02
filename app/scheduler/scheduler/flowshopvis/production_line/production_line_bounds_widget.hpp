#ifndef PRODUCTION_LINE_BOUNDS_WIDGET_H
#define PRODUCTION_LINE_BOUNDS_WIDGET_H

#include <solvers/broadcast_line_solver.hpp>

#include <QLayout>
#include <QScrollArea>
#include <QWidget>

class QLabel;

namespace FlowShopVis {
class ProductionLineBoundsWidget : public QWidget {
    Q_OBJECT
public:
    explicit ProductionLineBoundsWidget(QWidget *parent = nullptr);

    void setBounds(std::vector<algorithm::GlobalIntervals> bounds);

    void keyPressEvent(QKeyEvent *event) override;

public slots:
    void previousIteration();
    void nextIteration();

signals:
    void iterationChanged(std::size_t iteration);

protected:

private:
    std::size_t m_iteration = 0;
    std::size_t m_maxIterations = 0;

    QLabel *m_iterationLabel = nullptr;

    void updateLabel();
};
} // namespace FlowShopVis

#endif // PRODUCTION_LINE_BOUNDS_WIDGET_H