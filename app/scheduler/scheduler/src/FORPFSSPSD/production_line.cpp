#include "FORPFSSPSD/production_line.hpp"

using namespace FORPFSSPSD;

namespace {
std::optional<delay> getMapMaybe(const decltype(TransferPoint::dueDate) &table, JobId job) {
    const auto it = table.find(job);
    if (it == table.end()) {
        return std::nullopt;
    }

    return it->second;
}
} // namespace

ProductionLine ProductionLine::fromFlowShops(std::string problemName,
                                             std::unordered_map<ModuleId, Instance> modules,
                                             ModulesTransferConstraints transferConstraints) {

    // Get all keys
    std::vector<ModuleId> moduleIds;
    moduleIds.reserve(modules.size());
    for (const auto &[moduleId, _] : modules) {
        moduleIds.push_back(moduleId);
    }

    // Sort keys
    std::sort(moduleIds.begin(), moduleIds.end());

    // Check that keys are consecutive and that transfer constraints are as well.
    for (std::size_t i = 1; i < moduleIds.size(); ++i) {
        if (moduleIds[i] != moduleIds[i - 1] + 1) {
            throw std::runtime_error("Module IDs are not consecutive");
        }

        // We check that the transfer constraint from module i-1 to module i exists. Note that this
        // allows defining transfer constraints between other modules, they just get ignored.
        auto it = transferConstraints.find(moduleIds[i - 1]);
        if (it == transferConstraints.end()) {
            throw std::runtime_error(
                    fmt::format("No transfer constraints for module {}", moduleIds[i - 1]));
        }

        auto it2 = it->second.find(moduleIds[i]);
        if (it2 == it->second.end()) {
            throw std::runtime_error(
                    fmt::format("No transfer constraints from module {} to module {}",
                                moduleIds[i - 1],
                                moduleIds[i]));
        }
    }

    // Create modules
    std::unordered_map<ModuleId, Module> modulesMap;
    for (std::size_t i = 0; i < moduleIds.size(); ++i) {
        const auto moduleId = moduleIds[i];
        const auto previousModuleId = i == 0 ? std::nullopt : std::optional(moduleIds[i - 1]);
        const auto nextModuleId =
                i == moduleIds.size() - 1 ? std::nullopt : std::optional(moduleIds[i + 1]);
        modulesMap.emplace(moduleId,
                           Module{moduleId,
                                  previousModuleId,
                                  nextModuleId,
                                  i <= 0,
                                  std::move(modules.at(moduleId))});
    }

    // Create boundaries table
    BoundariesTable boundariesTable;
    for (std::size_t i = 1; i < moduleIds.size(); ++i) {
        const auto &module = modulesMap.at(moduleIds[i - 1]);
        const auto &transferPoint = transferConstraints(moduleIds[i - 1], moduleIds[i]);
        auto &boundModule = boundariesTable[moduleIds[i - 1]];

        // Create boundary for all jobs that come after the current job
        const auto &jobsOutput = module.getJobsOutput();
        for (std::size_t j1 = 0; j1 < jobsOutput.size(); ++j1) {
            const auto jobFrom = jobsOutput[j1];
            const auto opFrom = module.jobs(jobFrom).back();

            // To make it easy for the user, setup time and due dates are counted from the end of
            // operation to the start of the next one.
            const auto jobFProc = module.getProcessingTime(opFrom);
            const auto jobFST = transferPoint.setupTime(jobFrom) + jobFProc;
            const auto jobFDD = getMapMaybe(transferPoint.dueDate, jobFrom);

            if (jobFDD.has_value() && jobFDD.value() < jobFST) {
                throw std::domain_error(
                        fmt::format("Due date {} is smaller than setup time {} for job {}",
                                    jobFDD.value(),
                                    jobFST,
                                    jobFrom));
            }

            auto &boundJob = boundModule[jobFrom];

            for (std::size_t j2 = j1 + 1; j2 < jobsOutput.size(); ++j2) {
                const auto jobTo = jobsOutput[j2];
                const auto opTo = module.jobs(jobTo).back();

                const auto jobSProc = module.getProcessingTime(opTo);
                const auto jobSST = transferPoint.setupTime(jobTo) + jobSProc;
                const auto jobSDD = getMapMaybe(transferPoint.dueDate, jobTo);

                boundJob.emplace(jobTo, Boundary{jobFST, jobSST, jobFDD, jobSDD});
            }
        }
    }

    return {std::move(problemName),
            std::move(modulesMap),
            std::move(moduleIds),
            std::move(transferConstraints),
            std::move(boundariesTable)};
}
