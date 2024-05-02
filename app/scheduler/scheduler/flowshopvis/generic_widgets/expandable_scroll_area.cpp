#include "expandable_scroll_area.hpp"

#include <QScrollBar>

QSize FlowShopVis::ExpandableScrollArea::sizeHint() const {
    auto *const widget = this->widget();

    auto size = QScrollArea::sizeHint();

    if (widget == nullptr) {
        return size;
    }

    size.setWidth(widget->sizeHint().width() + 2 * frameWidth());

    auto *const scrollBar = verticalScrollBar();
    size.setWidth(size.width() + scrollBar->sizeHint().width());
    return size;
}
