#ifndef UTILS_DISTRIBUTED_SCHEDULER_HISTORY_HPP
#define UTILS_DISTRIBUTED_SCHEDULER_HISTORY_HPP

#include "FORPFSSPSD/bounds.hpp"
#include "FORPFSSPSD/production_line.hpp"
#include "solvers/production_line_solution.hpp"

#include <nlohmann/json.hpp>
#include <vector>

namespace FMS {

/// @brief History of the distributed schedulers
/// @details During the execution of the distributed schedulers, multiple bounds and schedules
/// are computed. This class contains tools to store it and save it as a JSON.
class DistributedSchedulerHistory {
public:
    DistributedSchedulerHistory(bool storeSequence, bool storeBounds) :
        m_storeSequence(storeSequence), m_storeBounds(storeBounds) {}

    void newIteration();

    void addIteration(const ModulesSolutions &modulesResults, const FS::GlobalBounds &allBounds);

    void addModule(FS::ModuleId moduleId,
                   const FS::ModuleBounds &bounds,
                   const PartialSolution &modResult);

    [[nodiscard]] inline nlohmann::json boundsToJSON() const {
        return FORPFSSPSD::toJSON(m_allBounds);
    }
    [[nodiscard]] nlohmann::json sequencesToJSON(const FS::ProductionLine &problem) const;

    [[nodiscard]] inline nlohmann::json toJSON(const FS::ProductionLine &problem) const {
        nlohmann::json json;
        if (!m_allResults.empty()) {
            json["sequences"] = sequencesToJSON(problem);
        }
        if (!m_allBounds.empty()) {
            json["bounds"] = boundsToJSON();
        }
        return json;
    }

private:
    std::vector<ModulesSolutions> m_allResults;
    std::vector<FS::GlobalBounds> m_allBounds;

    bool m_storeSequence;
    bool m_storeBounds;
};

} // namespace FMS

#endif // UTILS_DISTRIBUTED_SCHEDULER_HISTORY_HPP
