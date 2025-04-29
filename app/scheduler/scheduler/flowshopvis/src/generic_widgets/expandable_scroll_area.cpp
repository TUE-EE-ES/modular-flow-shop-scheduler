#include "expandable_scroll_area.hpp"

#include <QScrollBar>

QSize FlowShopVis::ExpandableScrollArea::sizeHint() const {
    auto *const widget = this->widget();

    auto size = QScrollArea::sizeHint();

    if (widget == nullptr) {
        return size;
    }

    auto *const scrollBar = verticalScrollBar();
    size.setWidth(widget->sizeHint().width() + 2 * frameWidth() + scrollBar->sizeHint().width());
    return size;
}
