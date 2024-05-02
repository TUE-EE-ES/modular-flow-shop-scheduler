
#include "layouts.hpp"

#include <QLayout>
#include <QWidget>

namespace FlowShopVis::Utils {

void clearLayout(QLayout *layout) {
    // From https://stackoverflow.com/a/4857631/4005637
    if (layout == nullptr) {
        return;
    }

    QLayoutItem *item = nullptr;
    while ((item = layout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    delete layout;
}
} // namespace FlowShopVis::Utils