#ifndef MODELS_BOUNDS_MODEL_H
#define MODELS_BOUNDS_MODEL_H

#include <FORPFSSPSD/module.hpp>
#include <solvers/broadcast_line_solver.hpp>

#include <QAbstractTableModel>

namespace FlowShopVis {
class BoundsModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit BoundsModel(QObject *parent = nullptr) : QAbstractTableModel(parent) {}

    void setBounds(std::vector<algorithm::ModuleBounds> bounds);

    // NOLINTBEGIN(google-default-arguments)
    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] inline int columnCount(const QModelIndex &parent = QModelIndex()) const override {
        return rowCount(parent);
    }
    [[nodiscard]] QVariant data(const QModelIndex &index,
                                int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant
    headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    // NOLINTEND(google-default-arguments)

    // NOLINTNEXTLINE: Slots is a Qt extension and the linter does not recognize it
public slots:
    void iterationChanged(std::size_t iteration);

private:
    std::vector<algorithm::ModuleBounds> m_bounds;

    std::vector<FORPFSSPSD::JobId> m_sortedJobs;

    std::vector<std::vector<bool>> m_highlight;

    std::size_t m_currentIndex = 0;

};
} // namespace FlowShopVis

#endif // MODELS_BOUNDS_MODEL_H