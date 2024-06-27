#include "bounds_model.hpp"

#include <algorithm>

#include <QFont>
#include <QColor>

using namespace FlowShopVis;

namespace {
[[nodiscard]] QVariant optionalToVariant(const std::optional<int> &opt, bool invert) {
    if (opt.has_value()) {
        return invert ? -opt.value() : opt.value();
    }
    return {};
}

QVariant getBound(const FORPFSSPSD::IntervalSpec &bounds,
                  const FORPFSSPSD::JobId jobIdFrom,
                  const FORPFSSPSD::JobId jobIdTo,
                  const bool invert) {

    const auto jFrom = invert ? jobIdTo : jobIdFrom;
    const auto jTo = invert ? jobIdFrom : jobIdTo;
    const auto it = bounds.find(jFrom);
    if (it == bounds.end()) {
        return {};
    }

    const auto &jobBounds = it->second;
    const auto it2 = jobBounds.find(jTo);
    if (it2 == jobBounds.end()) {
        return {};
    }

    const auto val = invert ? it2->second.max() : it2->second.min();
    return optionalToVariant(val, invert);
}

void highlightChanged(const FORPFSSPSD::IntervalSpec &oldBounds,
                      const FORPFSSPSD::IntervalSpec &newBounds,
                      const FORPFSSPSD::JobId jobFrom,
                      const FORPFSSPSD::JobId jobTo,
                      std::size_t posX,
                      std::size_t posY,
                      std::vector<std::vector<bool>> &highlight,
                      const bool invert) {

    const auto jFrom = invert ? jobTo : jobFrom;
    const auto jTo = invert ? jobFrom : jobTo;
    const auto itInOld = oldBounds.find(jFrom);
    const auto itInNew = newBounds.find(jFrom);

    if (itInOld == oldBounds.end() && itInNew != newBounds.end()) {
        const auto itNew = itInNew->second.find(jTo);
        if (itNew != itInNew->second.end()) {
            highlight.at(posX).at(posY) = true;
        }
        return;
    }

    if (itInOld != oldBounds.end() && itInNew != newBounds.end()) {
        const auto itOld = itInOld->second.find(jTo);
        const auto itNew = itInNew->second.find(jTo);
        if (itOld != itInOld->second.end() && itNew != itInNew->second.end()) {
            if (invert) {
                if (itOld->second.max() != itNew->second.max()) {
                    highlight.at(posX).at(posY) = true;
                }
            } else if (itOld->second.min() != itNew->second.min()) {
                highlight.at(posX).at(posY) = true;
            }
        }
    }
}

} // namespace

// NOLINTBEGIN(google-default-arguments)

void FlowShopVis::BoundsModel::setBounds(std::vector<FORPFSSPSD::ModuleBounds> bounds) {
    beginResetModel();
    m_bounds = std::move(bounds);
    m_currentIndex = 0;

    m_sortedJobs.clear();
    std::unordered_set<FORPFSSPSD::JobId> jobs;
    const auto &bound = m_bounds.at(m_currentIndex);
    for (const auto &[jobIdFrom, jobBounds] : bound.in) {
        jobs.insert(jobIdFrom);

        for (const auto &[jobIdTo, interval] : jobBounds) {
            jobs.insert(jobIdTo);
        }
    }

    for (const auto &[jobIdFrom, jobBounds] : bound.out) {
        jobs.insert(jobIdFrom);

        for (const auto &[jobIdTo, interval] : jobBounds) {
            jobs.insert(jobIdTo);
        }
    }

    m_sortedJobs.reserve(jobs.size());
    m_sortedJobs.insert(m_sortedJobs.end(), jobs.begin(), jobs.end());
    std::ranges::sort(m_sortedJobs);

    m_highlight.resize(m_sortedJobs.size() * 2);
    for (auto &row : m_highlight) {
        row.resize(m_sortedJobs.size() * 2);
    }
    // Set all to false
    for (auto &row : m_highlight) {
        std::fill(row.begin(), row.end(), false);
    }

    endResetModel();
}

int BoundsModel::rowCount(const QModelIndex &parent) const {

    if (parent.isValid()) {
        return 0;
    }

    // The number of rows is the total number of input and output operations in the module which
    // is twice the number of jobs
    return static_cast<int>(m_sortedJobs.size() * 2);
}

QVariant BoundsModel::data(const QModelIndex &index, int role) const {

    if (!index.isValid()) {
        return {};
    }

    if (role != Qt::DisplayRole && role != Qt::ToolTipRole && role != Qt::FontRole) {
        return {};
    }

    const auto row = static_cast<std::size_t>(index.row());
    const auto col = static_cast<std::size_t>(index.column());

    const auto jobFromIdx = col % m_sortedJobs.size();
    const auto jobToIdx = row % m_sortedJobs.size();
    const auto jobFrom = m_sortedJobs.at(jobFromIdx);
    const auto jobTo = m_sortedJobs.at(jobToIdx);

    if (role == Qt::ToolTipRole) {

        const auto sJobFrom = QString("J%1-").arg(jobFrom.value) + (jobFromIdx == col ? "I" : "O");
        const auto sJobTo = QString("J%1-").arg(jobTo.value) + (jobToIdx == row ? "I" : "O");
        return QString("From J%1 to J%2").arg(sJobFrom).arg(sJobTo);
    }

    if (row < 0 || row >= m_sortedJobs.size() * 2 || col < 0 || col >= m_sortedJobs.size() * 2) {
        return {};
    }

    if (role == Qt::FontRole) {
        QFont font;
        if (m_highlight.at(row).at(col)) {
            font.setBold(true);
        }
        return font;
    }
    
    // Display role
    if (row == col) {
        return 0;
    }

    const auto &bound = m_bounds.at(m_currentIndex);

    if (col < m_sortedJobs.size()) {
        // From input
        if (row >= m_sortedJobs.size()) {
            // To output. Input to output mapping still not implemented
            return {};
        }

        return getBound(bound.in, jobFrom, jobTo, jobFromIdx > jobToIdx);
    }

    // From output
    if (row < m_sortedJobs.size()) {
        // To input. Output to input mapping still not implemented
        return {};
    }
    // To output
    return getBound(bound.out, jobFrom, jobTo, jobFromIdx > jobToIdx);
}

QVariant BoundsModel::headerData(int section, Qt::Orientation /*orientation*/, int role) const {
    if (role != Qt::DisplayRole) {
        return {};
    }
    const auto jobIdx = section % m_sortedJobs.size();
    const auto jobId = m_sortedJobs.at(jobIdx);
    return QString("J%1-").arg(jobId.value) + (jobIdx == section ? "I" : "O");
}   

// NOLINTEND(google-default-arguments)

void FlowShopVis::BoundsModel::iterationChanged(std::size_t iteration) {
    const auto &oldBounds = m_bounds.at(m_currentIndex);
    m_currentIndex = iteration % m_bounds.size();
    const auto &newBounds = m_bounds.at(m_currentIndex);

    // Reset all highlighted
    for (auto &row : m_highlight) {
        std::fill(row.begin(), row.end(), false);
    }


    // Find values that are different
    for (std::size_t i = 0; i < m_sortedJobs.size(); ++i) {
        for (std::size_t j = 0; j < m_sortedJobs.size(); ++j) {
            const auto jobFrom = m_sortedJobs.at(i);
            const auto jobTo = m_sortedJobs.at(j);

            highlightChanged(oldBounds.in, newBounds.in, jobFrom, jobTo, j, i, m_highlight, i > j);
            highlightChanged(oldBounds.out,
                             newBounds.out,
                             jobFrom,
                             jobTo,
                             j + m_sortedJobs.size(),
                             i + m_sortedJobs.size(),
                             m_highlight,
                             i > j);
        }
    }

    emit dataChanged(index(0, 0), index(rowCount(), columnCount()));
}
