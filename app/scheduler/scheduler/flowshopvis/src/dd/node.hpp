#ifndef FLOWSHOPVIS_DD_NODE_HPP
#define FLOWSHOPVIS_DD_NODE_HPP

#include "graph/basic_node.hpp"

#include <QStaticText>

namespace FlowShopVis::DD {
class Node : public BasicNode {
    Q_OBJECT
public:
    explicit Node(QString name = QString(), qreal radius = kDefaultRadius);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QRectF boundingRect() const override;

private:
    QStaticText m_text;
};

} // namespace FlowShopVis::DD

#endif // FLOWSHOPVIS_DD_NODE_HPP