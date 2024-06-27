#ifndef SCHEDULER_SOLVER_DATA_HPP
#define SCHEDULER_SOLVER_DATA_HPP

#include <memory>

namespace FMS {

/**
 * @brief Data used by the solver.
 * @details The purpose of this class is to store the data that the solver uses to find a scheduler.
 * Each solver's data class should inherit it.
 */
struct SolverData {
    SolverData() = default;
    SolverData(const SolverData &) = default;
    SolverData(SolverData &&) = default;
    virtual ~SolverData() = default;

    SolverData& operator=(const SolverData &) = default;
    SolverData& operator=(SolverData &&) = default;
};

using SolverDataPtr = std::unique_ptr<SolverData>;
} // namespace FMS

#endif // SCHEDULER_SOLVER_DATA_HPP
