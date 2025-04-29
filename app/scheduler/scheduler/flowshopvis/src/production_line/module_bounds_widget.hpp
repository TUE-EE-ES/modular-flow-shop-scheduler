#ifndef MODULE_BOUNDS_WIDGET_H
#define MODULE_BOUNDS_WIDGET_H

#include "bounds_model.hpp"

#include <fms/problem/indices.hpp>

#include <QWidget>

namespace FlowShopVis {

class ModuleBoundsWidget : public QWidget {
    Q_OBJECT
public:
    explicit ModuleBoundsWidget(fms::problem::ModuleId moduleId, QWidget *parent = nullptr);

    inline void setBounds(std::vector<fms::problem::ModuleBounds> bounds) {
        m_model->setBounds(std::move(bounds));
    }

public slots:
    inline void iterationChanged(std::size_t iteration) {
        m_model->iterationChanged(iteration);
    }

private:
    BoundsModel *m_model;
    fms::problem::ModuleId m_moduleId;
};

} // namespace FlowShopVis

#endif // MODULE_BOUNDS_WIDGET_H