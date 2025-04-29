#ifndef FLOWSHOPVIS_DD_POSITIONS_WORKER_HPP
#define FLOWSHOPVIS_DD_POSITIONS_WORKER_HPP

#include <fms/dd/vertex.hpp>
#include <fms/solvers/dd.hpp>

#include <QPointF>
#include <QThread>

namespace FlowShopVis::DD {
/// @brief Worker class to calculate the positions of the nodes in the DD
class PositionsWorker : public QThread {
    Q_OBJECT
public:
    PositionsWorker(QObject *parent = nullptr) : QThread(parent) {}

    void setData(std::shared_ptr<fms::solvers::dd::DDSolverData> data) noexcept {
        m_data = std::move(data);
    }

    /// @brief Horizontal space that a node takes
    /// @details This includes the padding and the actual node size
    inline void setNodeXSpace(qreal nodeSpace) noexcept { m_nodeXSpace = nodeSpace; }

    /// @brief Space that a node takes including the padding and the actual node size
    /// @details This includes the padding and the actual node size
    inline void setNodeYSpace(qreal nodeSpace) noexcept { m_nodeYSpace = nodeSpace; }

    [[nodiscard]] inline std::unordered_map<fms::dd::VertexId, QPointF> &positions() noexcept {
        return m_positions;
    }

    [[nodiscard]] inline std::vector<std::vector<fms::dd::VertexId>> &edges() noexcept {
        return m_edges;
    }

    [[nodiscard]] inline std::optional<fms::dd::VertexId> rootId() const noexcept {
        return m_rootId;
    }

    void run() override;

signals:
    void positionsCalculated() const;
    void progress(qlonglong value) const;
    void error(const QString &message) const;

private:
    using NodesSize = std::unordered_map<fms::dd::VertexId, std::size_t>;

    qreal m_nodeXSpace = 0;
    qreal m_nodeYSpace = 0;

    std::shared_ptr<fms::solvers::dd::DDSolverData> m_data;

    // std::unordered_map<fms::DD::VertexId, std::deque<fms::DD::VertexId>> m_edges;
    std::vector<std::vector<fms::dd::VertexId>> m_edges;

    std::unordered_map<fms::dd::VertexId, QPointF> m_positions;

    std::optional<fms::dd::VertexId> m_rootId;

    [[nodiscard]] NodesSize computeNodesWidth(fms::dd::VertexId rootId) const;

    void calculatePositions(fms::dd::VertexId rootId, NodesSize nodesWidth);
};
} // namespace FlowShopVis::DD

#endif // FLOWSHOPVIS_DD_POSITIONS_THREAD_HPP
