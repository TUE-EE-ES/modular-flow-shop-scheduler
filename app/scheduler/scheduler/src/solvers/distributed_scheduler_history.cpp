#include "fms/pch/containers.hpp"

#include "fms/solvers/distributed_scheduler_history.hpp"

#include "fms/solvers/sequence.hpp"

using namespace fms;
using namespace fms::solvers;

void DistributedSchedulerHistory::newIteration() {
    if (m_storeSequence) {
        m_allResults.emplace_back();
    }

    if (m_storeBounds) {
        m_allBounds.emplace_back();
    }
}

void DistributedSchedulerHistory::addIteration(const ModulesSolutions &modulesResults,
                                               const problem::GlobalBounds &allBounds) {
    if (m_storeSequence) {
        m_allResults.emplace_back(modulesResults);
    }

    if (m_storeBounds) {
        m_allBounds.emplace_back(allBounds);
    }
}

void DistributedSchedulerHistory::addModule(const problem::ModuleId moduleId,
                                            const problem::ModuleBounds &bounds,
                                            const PartialSolution &modResult) {
    if (m_storeSequence) {
        if (m_allResults.empty()) {
            throw std::runtime_error("Trying to add module to empty history");
        }

        m_allResults.back().emplace(moduleId, modResult);
    }

    if (m_storeBounds) {
        if (m_allBounds.empty()) {
            throw std::runtime_error("Trying to add module to empty history");
        }

        m_allBounds.back().emplace(moduleId, bounds);
    }
}

nlohmann::json
DistributedSchedulerHistory::sequencesToJSON(const problem::ProductionLine &problem) const {
    nlohmann::json result;
    for (const auto &solution : m_allResults) {
        result.push_back(sequence::saveProductionLineSequences(solution, problem));
    }
    return result;
}
