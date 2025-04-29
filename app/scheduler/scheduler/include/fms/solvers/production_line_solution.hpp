#ifndef FMS_SOLVERS_PRODUCTION_LINE_SOLUTION_HPP
#define FMS_SOLVERS_PRODUCTION_LINE_SOLUTION_HPP

#include "partial_solution.hpp"

#include "fms/delay.hpp"

#include <unordered_map>

namespace fms::solvers {

using ModulesSolutions = std::unordered_map<problem::ModuleId, PartialSolution>;

using ProductionLineEdges = std::unordered_map<problem::ModuleId, MachineEdges>;

class ProductionLineSolution {
public:
    ProductionLineSolution(delay makespan, ModulesSolutions solutions) :
        m_makespan(makespan), m_solutions(std::move(solutions)) {
        static std::size_t id = 0;
        m_id = id++;
    }

    [[nodiscard]] delay getMakespan() const { return m_makespan; }

    [[nodiscard]] std::size_t getId() const { return m_id; }

    [[nodiscard]] const PartialSolution &operator[](const problem::ModuleId &module) const {
        return m_solutions.at(module);
    }

    [[nodiscard]] PartialSolution &operator[](const problem::ModuleId &module) {
        return m_solutions.at(module);
    }

    [[nodiscard]] inline const ModulesSolutions &getSolutions() const { return m_solutions; }

private:
    std::size_t m_id;

    delay m_makespan;

    ModulesSolutions m_solutions;
};
} // namespace fms::solvers

#endif // FMS_SOLVERS_PRODUCTION_LINE_SOLUTION_HPP