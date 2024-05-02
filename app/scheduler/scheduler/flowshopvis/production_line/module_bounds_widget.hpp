#ifndef MODULE_BOUNDS_WIDGET_H
#define MODULE_BOUNDS_WIDGET_H

#include "bounds_model.hpp"

#include <FORPFSSPSD/indices.hpp>

#include <QWidget>

namespace FlowShopVis {

class ModuleBoundsWidget : public QWidget {
    Q_OBJECT
public:
    explicit ModuleBoundsWidget(FORPFSSPSD::ModuleId moduleId, QWidget *parent = nullptr);

    inline void setBounds(std::vector<algorithm::ModuleBounds> bounds) {
        m_model->setBounds(std::move(bounds));
    }

public slots:
    inline void iterationChanged(std::size_t iteration) {
        m_model->iterationChanged(iteration);
    }

private:
    BoundsModel *m_model;
    FORPFSSPSD::ModuleId m_moduleId;
};

} // namespace FlowShopVis

#endif // MODULE_BOUNDS_WIDGET_H