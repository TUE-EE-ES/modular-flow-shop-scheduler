#ifndef CUSTOM_SCROLL_AREA_H
#define CUSTOM_SCROLL_AREA_H

#include <QScrollArea>

namespace FlowShopVis {

/// @class ExpandableScrollArea
/// @brief A scroll area that expands to fit its contents.
/// @details The default @ref QScrollArea expands until a maximum value. This class can expand
/// until fitting the contents.
class ExpandableScrollArea : public QScrollArea {
    Q_OBJECT
public:
    explicit ExpandableScrollArea(QWidget *parent = nullptr) : QScrollArea(parent) {}

    [[nodiscard]] QSize sizeHint() const override;
};

} // namespace FlowShopVis

#endif // CUSTOM_SCROLL_AREA_H