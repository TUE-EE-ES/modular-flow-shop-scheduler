#ifndef BackTrackSolver_H
#define BackTrackSolver_H

#include "FORPFSSPSD/production_line.hpp"
#include "production_line_solution.hpp"

#include "pch/containers.hpp"

#include <nlohmann/json.hpp>

namespace algorithm {

class BackTrackSolver {

public:
    static std::tuple<std::vector<FMS::ProductionLineSolution>, nlohmann::json>
    solve(FORPFSSPSD::ProductionLine &problemInstance, const commandLineArgs &args);

    static std::vector<PartialSolution>
    getFeasibleOptionsSorted(DelayGraph::delayGraph &dg,
                             const FORPFSSPSD::Instance &problem,
                             const DelayGraph::vertex &eligibleOperation,
                             const PartialSolution &solution,
                             const commandLineArgs &args,
                             bool fullCheck);
};
} // namespace algorithm
#endif // BackTrackSolver_H
