#ifndef CONSTRAINT_GRAPH_WIDGET_HPP
#define CONSTRAINT_GRAPH_WIDGET_HPP

#include "basic_graph_widget.hpp"
#include "dot_parser.hpp"

#include <fms/cg/edge.hpp>
#include <fms/problem/flow_shop.hpp>

#include <QString>

namespace FlowShopVis {
class OperationNode;
class Edge;

/// @class ConstraintGraphWidget
/// @brief A widget that provides a graphical representation of a flow shop.
///
/// This class inherits from QGraphicsView and provides functionality for adding nodes and edges,
/// zooming in and out, and handling key and wheel events.
class ConstraintGraphWidget : public BasicGraphWidget {
    Q_OBJECT
public:
    /// @brief Constructs a ConstraintGraphWidget with the given parent widget
    /// @param parent The parent widget. If it is nullptr, the widget will be a window.
    explicit ConstraintGraphWidget(QWidget *parent = nullptr);

    explicit ConstraintGraphWidget(const fms::cg::ConstraintGraph &dg,
                                   const DOT::ColouredEdges &highlighted = {},
                                   QWidget *parent = nullptr);

    /// @brief A list of colors used for nodes
    static const QList<QColor> colors;

    /// @brief Returns the nodes in the graph
    /// @return A reference to the nodes in the graph
    [[nodiscard]] inline const auto &getNodes() { return m_nodes; }

    /// @brief Adds a node to the graph
    /// @param moduleId The ID of the module
    /// @param operation The operation to be performed
    /// @param pos The position of the node
    /// @param color The color of the node
    /// @param boundingBox The bounding box of the node
    /// @return A pointer to the added node
    OperationNode *addNode(fms::problem::ModuleId moduleId,
                           fms::problem::Operation operation,
                           fms::cg::VertexId vertexId,
                           const QPointF &pos,
                           const QColor &color,
                           QRectF &boundingBox);

    /// @brief Adds a node to the graph
    /// @param moduleId The ID of the module
    /// @param operation The operation to be performed
    /// @param pos The position of the node
    /// @param color The color of the node
    /// @param boundingBox The bounding box of the node
    /// @param ASAP The ASAP time of the node
    /// @param ALAP The ALAP time of the node
    /// @return A pointer to the added node
    OperationNode *addNode(fms::problem::ModuleId moduleId,
                           fms::problem::Operation operation,
                           fms::cg::VertexId vertexId,
                           const QPointF &pos,
                           const QColor &color,
                           QRectF &boundingBox,
                           fms::delay ASAP,
                           fms::delay ALAP);

    /// @brief Adds an edge to the graph
    /// @param source The source node
    /// @param dest The destination node
    /// @param weight The weight of the edge
    /// @param angle The angle of the edge
    /// @param pen The pen used to draw the edge
    /// @param boundingBox The bounding box of the edge
    /// @return A pointer to the added edge
    Edge *addEdge(OperationNode *source,
                  OperationNode *dest,
                  fms::delay weight,
                  double angle,
                  const QPen &pen,
                  QRectF &boundingBox);

    /// @brief Adds edges between modules in the graph
    /// @param moduleId The ID of the module
    /// @param dg The delay graph
    /// @param boundingbox The bounding box of the module
    /// @param highlighted The edges to be highlighted
    void addModuleEdges(fms::problem::ModuleId moduleId,
                        const fms::cg::ConstraintGraph &dg,
                        QRectF &boundingbox,
                        const DOT::ColouredEdges &highlighted = {});

    /// @brief Returns a node in the graph
    /// @param moduleId The ID of the module
    /// @param operation The operation to be performed
    /// @return A reference to the node
    [[nodiscard]] inline const auto &getNode(fms::problem::ModuleId moduleId,
                                             fms::problem::Operation operation) {
        return m_nodes.at(moduleId).at(operation);
    }

    /// @brief Returns a color for a machine
    /// @param machine The machine
    /// @return The color for the machine
    [[nodiscard]] static QColor getColor(fms::problem::OperationId operation) {
        return colors[operation % colors.size()];
    }

signals:
    /// @brief Signal that is emitted when an operation is shown
    void showOperation(fms::problem::ModuleId, fms::problem::Operation, fms::cg::VertexId vertexId);

private:
    /// @brief The nodes in the graph
    fms::utils::containers::Map<
            fms::problem::ModuleId,
            fms::utils::containers::Map<fms::problem::Operation, OperationNode *>>
            m_nodes;
};
} // namespace FlowShopVis
#endif // CONSTRAINT_GRAPH_WIDGET_HPP
