#ifndef FLOWSHOPVIS_DD_DD_WIDGET_HPP
#define FLOWSHOPVIS_DD_DD_WIDGET_HPP

#include "graph/basic_graph_widget.hpp"
#include "node.hpp"
#include "positions_worker.hpp"

#include <fms/solvers/dd.hpp>

#include <QProgressDialog>
#include <memory>

namespace FlowShopVis::DD {

/**
 * @brief Plots a DD graph
 */
class GraphWidget : public BasicGraphWidget {
    Q_OBJECT
public:
    explicit GraphWidget(QWidget *parent);

    void setDDData(std::shared_ptr<fms::solvers::dd::DDSolverData> data);

    void clear() override;

private:
    Node *addNode(fms::dd::VertexId id, QPointF pos, QRectF &boundingBox);

    std::shared_ptr<fms::solvers::dd::DDSolverData> data;

    // std::unordered_map<fms::DD::VertexId, std::deque<fms::DD::VertexId>> m_edges;
    std::vector<std::vector<fms::dd::VertexId>> m_edges;
    std::optional<fms::dd::VertexId> m_rootId;

    PositionsWorker *m_positionsWorker = nullptr;
    QProgressDialog *m_progressDialog = nullptr;

private slots:
    void positionsCalculated();
    void error(const QString &message);
};

} // namespace FlowShopVis::DD

#endif // FLOWSHOPVIS_GRAPH_DD_DD_WIDGET_HPP
