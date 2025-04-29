#ifndef NODE_H
#define NODE_H

#include "basic_node.hpp"

#include <fms/delay.hpp>
#include <fms/problem/operation.hpp>
#include <fms/cg/edge.hpp>

#include <QGraphicsItem>
#include <QList>
#include <QStaticText>

class QGraphicsSceneMouseEvent;

namespace FlowShopVis {
class Edge;

class OperationNode : public BasicNode {
    Q_OBJECT
public:
    // Constructor for the OperationNode class.
    // @param a_operation: An operation object from the FORPFSSPSD library.
    // @param radius: The radius of the node. Default is 15.
    // @pram ASAP: The ASAP delay of the node. Default is 15.
    explicit OperationNode(fms::problem::Operation a_operation,
                           fms::cg::VertexId vertexId,
                           qreal radius = BasicNode::kDefaultRadius);

    /// @brief Set As Soon As Possible time of the node
    void setASAP(fms::delay ASAP);

    /// @brief Set As Late As Possible time of the node
    void setALAP(fms::delay ALAP);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QRectF boundingRect() const override;

protected:
    void updateTextPositions();

    void adjustTextSize();

private:
    const fms::problem::Operation m_operation;
    const fms::cg::VertexId m_vertexId;
    QStaticText m_ASAP;
    QStaticText m_ALAP;

signals:
    /// @brief Emitted when the node is selected
    void operationSelected(fms::problem::Operation operation, fms::cg::VertexId vertexID);
};
} // namespace FlowShopVis

#endif // NODE_H
