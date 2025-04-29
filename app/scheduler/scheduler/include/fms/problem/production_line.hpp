#ifndef FMS_PROBLEM_PRODUCTION_LINE_HPP
#define FMS_PROBLEM_PRODUCTION_LINE_HPP

#include "indices.hpp"
#include "module.hpp"

#include "fms/delay.hpp"
#include "fms/utils/default_map.hpp"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace fms::problem {

/**
 * @brief Models the transfer constraints between modules.
 */
struct TransferPoint {
    /**
     * @brief Setup time for each job.
     * @details The transfer setup time \f$ts : J \rightarrow \mathbb{R}\f$ of a job \f$j\f$ is the
     * minimum time between the **end** of the last operation of the job in the previous module
     * \f$\Omega(lst_x(j)) + prc(lst_x(j)\f$ and the **start** of the first operation of the job in
     * the next module \f$\Omega(fst_{x+1}(j))\f$.
     */
    utils::containers::DefaultMap<JobId, delay> setupTime;

    /**
     * @brief Transfer due date for each job.
     * @details The transfer due date \f$td : J \hookrightarrow \mathbb{R}\f$ of a job \f$j\f$ is
     * the maximum time between the **start** of the last operation of the job in the previous
     * module \f$\Omega(lst_x(j))\f$ and the **start** of the first operation of the job in the
     * next module \f$\Omega(fst_{x+1}(j))\f$.
     * @note The XML input file defines the due date as the maximum time between the **end** of the
     * last operation of the job in the previous module \f$\Omega(lst_x(j))+prc(lst_x(j))\f$ and
     * the **start** of the first operation of the job in the next module
     * \f$\Omega(fst_{x+1}(j))\f$. This is done to make it consistent with the setup time.
     */
    std::unordered_map<JobId, delay> dueDate;
};

using ModulesTransferConstraints = utils::containers::TwoKeyMap<ModuleId, TransferPoint>;

class ProductionLine {
public:
    using BoundariesTable =
            std::unordered_map<ModuleId,
                               std::unordered_map<JobId, std::unordered_map<JobId, Boundary>>>;

    static ProductionLine fromFlowShops(std::string problemName,
                                        std::unordered_map<ModuleId, Instance> modules,
                                        ModulesTransferConstraints transferConstraints);

    [[nodiscard]] inline std::size_t getNumberOfJobs() const {
        return m_modules.begin()->second.getNumberOfJobs();
    }

    [[nodiscard]] std::size_t getNumberOfMachines() const {
        std::size_t total = 0;
        for (const auto &[_, module] : m_modules) {
            total += module.getNumberOfMachines();
        }
        return total;
    }

    [[nodiscard]] const std::string &getProblemName() const noexcept { return m_problemName; }

    [[nodiscard]] inline std::unordered_map<ModuleId, Module> &modules() noexcept {
        return m_modules;
    }

    [[nodiscard]] inline const std::unordered_map<ModuleId, Module> &modules() const noexcept {
        return m_modules;
    }

    [[nodiscard]] inline const std::vector<ModuleId> &moduleIds() const noexcept {
        return m_moduleIds;
    }

    [[nodiscard]] inline const ModulesTransferConstraints &getTransferConstraints() const noexcept {
        return m_transferConstraints;
    }

    /**
     * @brief Get the transfer setup time for a job.
     * @details The transfer setup time \f$ts : J \rightarrow \mathbb{R}\f$ of a job \f$j\f$ is the
     * minimum time between the **end** of the last operation of the job in the previous module
     * \f$\Omega(lst_x(j)) + prc(lst_x(j)\f$ and the **start** of the first operation of the job in
     * the next module \f$\Omega(fst_{x+1}(j))\f$.
     * @tparam T @ref Module or @ref ModuleId
     * @param from Reference to the module from which the job is being transferred.
     * @param jobId Job being transferred.
     * @return delay \f$ts(j)\f$
     */
    template <typename T> [[nodiscard]] inline delay getTransferSetup(T &&from, JobId jobId) const {
        const auto moduleIdFrom = getModuleId(from);
        const auto moduleIdTo = moduleIdFrom + 1; // Assume consecutive modules
        return m_transferConstraints(moduleIdFrom, moduleIdTo).setupTime(jobId);
    }

    /**
     * @brief Get minimum transfer time.
     * @details The minimum transfer time of a job \f$j\f$ is the minimum time between the
     * **start** of the last operation of the job in the previous module \f$\Omega(lst_x(j))\f$ and
     * the **start** of the first operation of the job in the next module
     * \f$\Omega(fst_{x+1}(j))\f$. It's value is:
     * \f[
     * delay(x, j) = prc_x(lst_x(j)) + ts_x(j)
     * \f]
     * @tparam T @ref Module or @ref ModuleId
     * @param from Reference to the module from which the job is being transferred.
     * @param jobId Job being transferred.
     * @return delay \f$prc_x(lst_x(j)) + ts_x(j)\f$
     */
    template <typename T> [[nodiscard]] delay query(T &&from, JobId jobId) const {
        const auto &moduleFrom = getModule(from);
        const delay setup = getTransferSetup(from, jobId);
        const auto &opLast = moduleFrom.jobs(jobId).back();
        return moduleFrom.getProcessingTime(opLast) + setup;
    }

    template <typename T>
    [[nodiscard]] std::optional<delay> getTransferDueDate(T &&from, JobId jobId) const {
        const auto moduleIdFrom = getModuleId(from);
        const auto moduleIdTo = moduleIdFrom + 1; // Assume consecutive modules
        const auto &dueDates = m_transferConstraints(moduleIdFrom, moduleIdTo).dueDate;
        auto it = dueDates.find(jobId);

        return it == dueDates.end() ? std::nullopt : std::make_optional(it->second);
    }

    /// Returns the module with ID equal to `id` constant version. Note, the problem needs to be
    /// modularised first.
    inline const Module &operator[](ModuleId id) const { return m_modules.at(id); }
    inline Module &operator[](ModuleId id) { return m_modules.at(id); }
    [[nodiscard]] inline const Module &getModule(ModuleId id) const { return m_modules.at(id); }
    [[nodiscard]] inline Module &getModule(ModuleId id) { return m_modules.at(id); }
    [[nodiscard]] static inline const Module &getModule(const Module &mod) { return mod; }
    [[nodiscard]] static inline Module &getModule(Module &mod) { return mod; }

    template <typename T> [[nodiscard]] inline ModuleId getModuleId(T &&mod) const {
        return getModule(mod).getModuleId();
    }
    [[nodiscard]] static inline ModuleId getModuleId(ModuleId id) { return id; }

    [[nodiscard]] inline size_t getNumberOfModules() const { return m_modules.size(); }

    template <typename T> [[nodiscard]] inline bool hasPrevModule(const T &module) const {
        return getModule(module).getPrevModuleId().has_value();
    }

    template <typename T> [[nodiscard]] inline Module &getPrevModule(const T &module) {
        return m_modules.at(getPrevModuleId(module));
    }

    template <typename T> [[nodiscard]] inline const Module &getPrevModule(const T &module) const {
        return m_modules.at(getPrevModuleId(module));
    }

    template <typename T> [[nodiscard]] inline ModuleId getPrevModuleId(const T &module) const {
        return getModule(module).getPrevModuleId().value();
    }

    template <typename T> [[nodiscard]] inline bool hasNextModule(const T &module) const {
        return getModule(module).getNextModuleId().has_value();
    }

    template <typename T> [[nodiscard]] inline Module &getNextModule(const T &module) {
        return m_modules.at(getNextModuleId(module));
    }

    template <typename T> [[nodiscard]] inline const Module &getNextModule(const T &module) const {
        return m_modules.at(getNextModuleId(module));
    }

    template <typename T> [[nodiscard]] inline ModuleId getNextModuleId(const T &module) const {
        return getModule(module).getNextModuleId().value();
    }

    [[nodiscard]] inline problem::ModuleId getFirstModuleId() const { return m_moduleIds.front(); }

    [[nodiscard]] inline const problem::Module &getFirstModule() const {
        return m_modules.at(getFirstModuleId());
    }

    [[nodiscard]] inline problem::ModuleId getLastModuleId() const { return m_moduleIds.back(); }

    [[nodiscard]] inline const problem::Module &getLastModule() const {
        return m_modules.at(getLastModuleId());
    }

    /**
     * @brief Translates the given intervals to intervals valid at the input boundary of the passed
     * module and returns the translated version.
     * @pre The intervals correspond to output intervals of the module before @p module
     *
     * @param module Destination module that will receive the returned intervals.
     * @param intervals Intervals corresponding to the output boundary of the module before
     * @p module
     * @return IntervalSpec Translated intervals valid at the input boundary of @p module .
     */
    template <typename T>
    [[nodiscard]] inline IntervalSpec toInputBounds(T &&module,
                                                    const IntervalSpec &intervals) const {
        return translateIntervals<&Boundary::translateToDestination>(getPrevModuleId(module),
                                                                     intervals);
    }

    /**
     * @brief Translates the given intervals to intervals valid at the output boundary of the passed
     * module and returns the translated version.
     * @pre The intervals correspond to input intervals of the module after @p module
     *
     * @param module Destination module that will receive the returned intervals.
     * @param intervals Intervals corresponding to the input boundary of the module after
     * @p module
     * @return IntervalSpec Translated intervals valid at the output boundary of @p module .
     */
    template <typename T>
    [[nodiscard]] inline IntervalSpec toOutputBounds(T &&module,
                                                     const IntervalSpec &intervals) const {
        return translateIntervals<&Boundary::translateToSource>(module, intervals);
    }

    using FBoundaryInterval = TimeInterval (Boundary::*)(const TimeInterval &) const;

    template <FBoundaryInterval F, typename T>
    [[nodiscard]] IntervalSpec translateIntervals(T &&module, const IntervalSpec &intervals) const {
        IntervalSpec result;
        const auto &boundModule = m_boundaries.at(getModuleId(module));

        for (const auto &[jobFstId, jobFstIntervals] : intervals) {
            const auto &boundJobFst = boundModule.at(jobFstId);
            for (const auto &[jobSndId, interval] : jobFstIntervals) {
                const auto &boundary = boundJobFst.at(jobSndId);
                // Weird syntax to call a member function pointer
                try {
                    auto newInterval = (boundary.*F)(interval);
                    result[jobFstId].emplace(jobSndId, newInterval);
                } catch (BoundaryTranslationError &e) {
                    throw FmsSchedulerException(e.what());
                }
            }
        }

        return result;
    }

private:
    /// Name of the problem
    std::string m_problemName;

    /// All the modules in the production line
    std::unordered_map<ModuleId, Module> m_modules;

    /// IDs of all the modules in order
    std::vector<ModuleId> m_moduleIds;

    /// Transfer constraints between modules
    ModulesTransferConstraints m_transferConstraints;

    /// Boundaries between modules
    BoundariesTable m_boundaries;

    ProductionLine(std::string problemName,
                   std::unordered_map<ModuleId, Module> modules,
                   std::vector<ModuleId> moduleIds,
                   ModulesTransferConstraints transferConstraints,
                   BoundariesTable boundaries) :
        m_problemName(std::move(problemName)),
        m_modules(std::move(modules)),
        m_moduleIds(std::move(moduleIds)),
        m_transferConstraints(std::move(transferConstraints)),
        m_boundaries(std::move(boundaries)) {}
};

} // namespace fms::problem

#endif // FMS_PROBLEM_PRODUCTION_LINE_HPP
