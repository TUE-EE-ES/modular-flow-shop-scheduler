#include "node.hpp"

#include <QPainter>
#include <QStyleOptionGraphicsItem>

using namespace FlowShopVis::DD;

Node::Node(QString name, qreal radius) : BasicNode(radius), m_text(name) {}

void Node::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    BasicNode::paint(painter, option, widget);

    const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

    if (lod >= 0.7) {
        const auto size = m_text.size();
        painter->drawStaticText(-size.width() / 2, -size.height() / 2, m_text);
    }
}

QRectF Node::boundingRect() const {
    const auto size = m_text.size();
    QRectF textRect(QPointF(-size.width() / 2, -size.height() / 2), size);
    return BasicNode::boundingRect().united(textRect); 
}