#ifndef SCHEDULER_SOLVER_DATA_HPP
#define SCHEDULER_SOLVER_DATA_HPP

#include "../utils/pointers.hpp"

#include <memory>

namespace fms::solvers {

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

template <typename Derived> std::unique_ptr<Derived> castSolverData(SolverDataPtr data) {
    if (data == nullptr) {
        return nullptr;
    }

    auto result = utils::memory::dynamic_unique_ptr_cast<Derived>(data);
    if (result == nullptr) {
        throw std::runtime_error("Unable to cast solver data to derived type");
    }
    return result;
}
} // namespace fms::solvers

#endif // SCHEDULER_SOLVER_DATA_HPP
