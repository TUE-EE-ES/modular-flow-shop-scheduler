//
// Created by 20204729 on 05/05/2021.
//

#ifndef FMS_PROBLEM_MODULE_HPP
#define FMS_PROBLEM_MODULE_HPP

#include "boundary.hpp"
#include "bounds.hpp"
#include "flow_shop.hpp"
#include "indices.hpp"
#include "operation.hpp"

#include <optional>
#include <set>
#include <string>

namespace fms::problem {

/// Relates two jobs to the interval between them.

struct ModuleInfo {
    // Initialized by the XML parser class

    /// @brief ID of the module. Unique for among all modules.
    ModuleId id;

    /** @brief Set of machines assigned to the module in global ID */
    std::set<MachineId> machines;

    // Not initialized by the XML parser class

    /** @brief Flow vector of all the jobs */
    std::vector<MachineId> flowVector;

    /** @brief Plexity of the jobs in the module */
    PlexityTable jobPlexity;

    /** @brief Index of previous module that a job is travelling from */
    std::optional<ModuleId> previousModuleId;

    /** @brief Index of next module that a job travels to */
    std::optional<ModuleId> nextModuleId;
};

/// @brief This class represents a module in the FORPFSSPSD system.
/// It extends the Instance class and contains additional information about the module.
class Module : public Instance {
public:
    /**
     * @brief Constructor for the Module class.
     * @tparam Args
     * @param moduleId The ID of the module.
     * @param machines The machines assigned to the module.
     * @param previousModule The previous module that a job travels from.
     * @param nextModule The next module that a job travels to.
     * @param problemName The name of the problem.
     * @param args Additional arguments.
     */
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

    /**
     * @brief Constructor for the Module class.
     * @param moduleId The ID of the module.
     * @param previousModule The previous module that a job travels from.
     * @param nextModule The next module that a job travels to.
     * @param outOfOrder Whether the module is out of order.
     * @param instance An instance of the problem.
     */
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

    /**
     * @brief Get the ID of the module.
     * @return
     */
    [[nodiscard]] inline ModuleId getModuleId() const { return m_id; }

    /**
     * @brief add bounds for the inputs based on the intervals
     * @param intervals The intervals to add
     */
    void addInputBounds(const IntervalSpec &intervals);

    /**
     * @brief add bounds for the outputs based on the intervals
     * @param intervals The intervals to add
     */
    void addOutputBounds(const IntervalSpec &intervals);

    /**
     * @brief Add an interval for two operations
     * @param from The operation the interval starts from
     * @param to The operation the interval ends at
     * @param value The interval to add
     */
    void addInterval(const Operation &from, const Operation &to, const TimeInterval &value);

    template <typename T> void setIteration(const T &iteration) {
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
} // namespace fms::problem

#endif // FMS_PROBLEM_MODULE_HPP
