#ifndef FMS_SOLVERS_DISTRIBUTED_SCHEDULER_HISTORY_HPP
#define FMS_SOLVERS_DISTRIBUTED_SCHEDULER_HISTORY_HPP

#include "fms/problem/bounds.hpp"
#include "fms/problem/production_line.hpp"
#include "fms/solvers/algorithms_data.hpp"
#include "fms/solvers/production_line_solution.hpp"

#include <nlohmann/json.hpp>
#include <vector>

namespace fms::solvers {

/// @brief History of the distributed schedulers
/// @details During the execution of the distributed schedulers, multiple bounds and schedules
/// are computed. This class contains tools to store it and save it as a JSON.
class DistributedSchedulerHistory {
public:
    DistributedSchedulerHistory(bool storeSequence, bool storeBounds) :
        m_storeSequence(storeSequence), m_storeBounds(storeBounds) {}

    void newIteration();

    void addIteration(const ModulesSolutions &modulesResults,
                      const problem::GlobalBounds &allBounds);

    void addModule(problem::ModuleId moduleId,
                   const problem::ModuleBounds &bounds,
                   const PartialSolution &modResult);

    inline void addAlgorithmData(const problem::ModuleId moduleId, nlohmann::json data) {
        m_algorithmsData.addData(moduleId, std::move(data));
    }

    [[nodiscard]] inline nlohmann::json boundsToJSON() const {
        return problem::toJSON(m_allBounds);
    }

    [[nodiscard]] nlohmann::json sequencesToJSON(const problem::ProductionLine &problem) const;

    [[nodiscard]] inline nlohmann::json toJSON(const problem::ProductionLine &problem) const {
        nlohmann::json json;
        if (!m_allResults.empty()) {
            json["sequences"] = sequencesToJSON(problem);
        }
        if (!m_allBounds.empty()) {
            json["bounds"] = boundsToJSON();
        }
        json["algorithmsData"] = m_algorithmsData.toJSON();
        return json;
    }

private:
    std::vector<ModulesSolutions> m_allResults;
    std::vector<problem::GlobalBounds> m_allBounds;
    AlgorithmsData m_algorithmsData;

    bool m_storeSequence;
    bool m_storeBounds;
};

} // namespace fms::solvers

#endif // FMS_SOLVERS_DISTRIBUTED_SCHEDULER_HISTORY_HPP
