#ifndef PRODUCTION_LINE_SOLUTION_HPP
#define PRODUCTION_LINE_SOLUTION_HPP

#include "delay.h"
#include "partialsolution.h"

#include <unordered_map>

namespace FMS {

using ModulesSolutions = std::unordered_map<FORPFSSPSD::ModuleId, PartialSolution>;

using ProductionLineSequences =
        std::unordered_map<FORPFSSPSD::ModuleId, PartialSolution::MachineEdges>;
class ProductionLineSolution {
public:
    ProductionLineSolution(delay makespan, ModulesSolutions solutions) :
        m_makespan(makespan), m_solutions(std::move(solutions)) {
        static std::size_t id = 0;
        m_id = id++;
    }

    [[nodiscard]] delay getMakespan() const { return m_makespan; }

    [[nodiscard]] std::size_t getId() const { return m_id; }

    [[nodiscard]] const PartialSolution &operator[](const FORPFSSPSD::ModuleId &module) const {
        return m_solutions.at(module);
    }

    [[nodiscard]] PartialSolution &operator[](const FORPFSSPSD::ModuleId &module) {
        return m_solutions.at(module);
    }

    [[nodiscard]] inline const ModulesSolutions &getSolutions() const { return m_solutions; }

private:
    std::size_t m_id;

    delay m_makespan;

    ModulesSolutions m_solutions;
};
} // namespace FMS

#endif // PRODUCTION_LINE_SOLUTION_HPP