#include "pch/containers.hpp"

#include "utils/distributed_scheduler_history.hpp"

#include "solvers/sequence.hpp"

using namespace FMS;

void DistributedSchedulerHistory::newIteration() {
    if (m_storeSequence) {
        m_allResults.emplace_back();
    }

    if (m_storeBounds) {
        m_allBounds.emplace_back();
    }
}

void FMS::DistributedSchedulerHistory::addIteration(const ModulesSolutions &modulesResults,
                                                    const FS::GlobalBounds &allBounds) {
    if (m_storeSequence) {
        m_allResults.emplace_back(modulesResults);
    }

    if (m_storeBounds) {
        m_allBounds.emplace_back(allBounds);
    }
}

void FMS::DistributedSchedulerHistory::addModule(const FS::ModuleId moduleId,
                                                 const FS::ModuleBounds &bounds,
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
FMS::DistributedSchedulerHistory::sequencesToJSON(const FS::ProductionLine &problem) const {
    nlohmann::json result;
    for (const auto &solution : m_allResults) {
        result.push_back(algorithm::Sequence::saveProductionLineSequences(solution, problem));
    }
    return result;
}
