#ifndef CUSTOM_SCROLL_AREA_H
#define CUSTOM_SCROLL_AREA_H

#include <QScrollArea>

namespace FlowShopVis {

/// A scroll area that expands to fit its contents.
class ExpandableScrollArea : public QScrollArea {
    Q_OBJECT
public:
    explicit ExpandableScrollArea(QWidget *parent = nullptr) : QScrollArea(parent) {}

    [[nodiscard]] QSize sizeHint() const override;
};

} // namespace FlowShopVis

#endif // CUSTOM_SCROLL_AREA_H