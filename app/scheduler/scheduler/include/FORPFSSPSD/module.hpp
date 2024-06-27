//
// Created by 20204729 on 05/05/2021.
//

#ifndef MODULE_HPP
#define MODULE_HPP

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "FORPFSSPSD/operation.h"
#include "boundary.hpp"
#include "bounds.hpp"
#include "indices.hpp"

#include <optional>
#include <set>
#include <string>

namespace FORPFSSPSD {

/// Relates two jobs to the interval between them.

/**
 * @brief Basic struct containing information about a module needed in order to initialize it.
 */
struct ModuleInfo {
    // Initialized by the XML parser class

    /// ID of the module. Unique for among all modules.
    ModuleId id;

    /// Set of machines assigned to the module in global ID
    std::set<MachineId> machines;

    // Not initialized by the XML parser class

    /// Flow vector of all the jobs
    std::vector<MachineId> flowVector;

    /// Plexity of the jobs in the module
    PlexityTable jobPlexity;

    /// Index of previous module that a job is travelling from
    std::optional<ModuleId> previousModuleId;

    /// Index of next module that a job travels to
    std::optional<ModuleId> nextModuleId;
};

class Module : public Instance {
public:
    template <typename... Args>
    Module(ModuleId moduleId,
           std::set<MachineId> machines,
           std::optional<ModuleId> previousModule,
           std::optional<ModuleId> nextModule,
           std::string problemName,
           Args &&...args) :
        Instance(fmt::format("{}_{:d}", problemName, moduleId), std::forward<Args>(args)...),
        m_id(moduleId),
        m_machines(std::move(machines)),
        m_previousModule(previousModule),
        m_nextModule(nextModule),
        m_originalName(fmt::format("{}_{:d}", problemName, moduleId)) {}

    Module(ModuleId moduleId,
           std::optional<ModuleId> previousModule,
           std::optional<ModuleId> nextModule,
           bool outOfOrder,
           Instance instance) :
        Instance(std::move(instance)),
        m_id(moduleId),
        m_previousModule(previousModule),
        m_nextModule(nextModule),
        m_originalName(fmt::format("{}_{:d}", Instance::getProblemName(), moduleId)) {
        setProblemName(m_originalName);
        setOutOfOrder(outOfOrder);
    }

    Module(const Module &) = default;
    Module(Module &&) noexcept = default;
    ~Module() override = default;

    Module &operator=(const Module &) = default;
    Module &operator=(Module &&) noexcept = default;

    /**
     * @brief Whether jobs come from another module or this is the first one.
     *
     * @return true Jobs come from a previous module.
     * @return false Jobs start being processed at this module.
     */
    [[nodiscard]] inline bool hasPrevModule() const { return m_previousModule.has_value(); }

    /**
     * @brief Get the index of the previous module where jobs are travelling from.
     *
     * @return const std::optional<ModuleId>& Index of previous module. If there is no previous
     * module the optional has no value.
     */
    [[nodiscard]] inline const std::optional<ModuleId> &getPrevModuleId() const {
        return m_previousModule;
    }

    /**
     * @brief Whether jobs travel to another module or this is the last one.
     *
     * @return true Jobs travel to a next module.
     * @return false This is the last module where jobs are being processed.
     */
    [[nodiscard]] inline bool hasNextModule() const { return m_nextModule.has_value(); }

    /**
     * @brief Get the index of the next module where job are travelling to.
     *
     * @return const std::optional<ModuleId>& Index of next module. If there is no next module, the
     * optional has no value.
     */
    [[nodiscard]] inline const std::optional<ModuleId> &getNextModuleId() const {
        return m_nextModule;
    }

    [[nodiscard]] inline ModuleId getModuleId() const { return m_id; }

    void addInputBounds(const IntervalSpec &intervals);

    void addOutputBounds(const IntervalSpec &intervals);

    void addInterval(const operation &from, const operation &to, const TimeInterval &value);

    template<typename T> void setIteration(const T& iteration) {
        setProblemName(fmt::format("{}_{}", m_originalName, iteration));
    }

private:
    /// ID of the module
    ModuleId m_id;

    /// Set of machines under the domain of this module
    std::set<MachineId> m_machines;

    /// Previous module where jobs will flow from
    std::optional<ModuleId> m_previousModule;

    /// Next module where jobs will flow to
    std::optional<ModuleId> m_nextModule;

    std::string m_originalName;
};
} // namespace FORPFSSPSD

#endif // MODULE_HPP
