#define _USE_MATH_DEFINES
#include "operation_node.hpp"

#include "edge.hpp"

#include <fms/algorithms/longest_path.hpp>
#include <fms/delay.hpp>

#include <QFontMetrics>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

using namespace FlowShopVis;

OperationNode::OperationNode(fms::problem::Operation a_operation, fms::cg::VertexId vertexId, qreal radius) :
    BasicNode(radius), m_operation(a_operation), m_vertexId(vertexId) {
    connect(this, &OperationNode::selected, this, [this]() {
        emit operationSelected(m_operation, m_vertexId);
    });
}

void OperationNode::setASAP(fms::delay ASAP) {
    if (ASAP == fms::algorithms::paths::kASAPStartValue) {
        m_ASAP.setText(QString::fromUtf8("\u221E"));
    } else {
        m_ASAP.setText(QString::number(ASAP));
    }
    adjustTextSize();
    updateTextPositions();
    update();
}

void OperationNode::setALAP(fms::delay ALAP) {
    if (ALAP == fms::algorithms::paths::kALAPStartValue) {
        m_ALAP.setText(QString::fromUtf8("-\u221E"));
    } else {
        m_ALAP.setText(QString::number(ALAP));
    }
    adjustTextSize();
    updateTextPositions();
    update();
}

void OperationNode::paint(QPainter *painter,
                          const QStyleOptionGraphicsItem *option,
                          QWidget *widget) {
    BasicNode::paint(painter, option, widget);

    const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

    if (lod >= 0.7) {
        painter->drawStaticText(-m_ASAP.size().width() / 2, m_ASAP.size().height(), m_ASAP);
        painter->drawStaticText(-m_ALAP.size().width() / 2, -m_ALAP.size().height(), m_ALAP);
    }
}

QRectF OperationNode::boundingRect() const {
    QRectF asapRect(QPointF(-m_ASAP.size().width() / 2, m_ASAP.size().height()), m_ASAP.size());
    QRectF alapRect(QPointF(-m_ALAP.size().width() / 2, m_ALAP.size().height()), m_ALAP.size());
    return BasicNode::boundingRect().united(asapRect).united(alapRect);
}

// function to update the position of the ASAP and ALAP times
// inside the nodes in the constraint graph
void OperationNode::updateTextPositions() {
    // Calculate the bounding boxes
    // QRectF asapRect = m_ASAP->boundingRect();
    // QRectF alapRect = m_ALAP->boundingRect();

    // // height of the node rectangle
    // qreal nodeRectHeight = boundingRect().height();
    // // total height of the ASAP and ALAP text
    // qreal totalHeight = asapRect.height() + alapRect.height();
    // qreal offset = (nodeRectHeight - totalHeight) * 5 / 12;

    // // set the y position of the ASAP and ALAP text
    // qreal asapY = -asapRect.height() - offset;
    // qreal alapY = offset;

    // m_ASAP->setPos(-asapRect.width() / 2, asapY);
    // m_ALAP->setPos(-alapRect.width() / 2, alapY);
}

// Function to adjust the text size of the ASAP and ALAP times
// so that they fit within the node
void OperationNode::adjustTextSize() {
    // QFont font = m_ASAP->font();
    // constexpr const int kMaxFontSize = 10;
    // int fontSize = kMaxFontSize; // Starting font size
    // font.setPointSize(fontSize);

    // // Set the font for both text items
    // m_ASAP->setFont(font);
    // m_ALAP->setFont(font);

    // QString asapText = m_ASAP->toPlainText();
    // QString alapText = m_ALAP->toPlainText();

    // QFontMetrics metrics(font);
    // QRectF asapRect = metrics.boundingRect(asapText);
    // QRectF alapRect = metrics.boundingRect(alapText);

    // // Adjust font size until the text fits within the node
    // constexpr const qreal kRadiusRatio = 1.8;
    // const auto radiusDistance = radius() * kRadiusRatio;
    // while ((asapRect.width() > radiusDistance || alapRect.width() > radiusDistance
    //         || asapRect.height() + alapRect.height() > radiusDistance)
    //        && fontSize > 1) {
    //     fontSize--;
    //     font.setPointSize(fontSize);

    //     metrics = QFontMetrics(font);
    //     asapRect = metrics.boundingRect(asapText);
    //     alapRect = metrics.boundingRect(alapText);
    // }

    // m_ASAP->setFont(font);
    // m_ALAP->setFont(font);
}
