#ifndef SIMPLESCHEDULER_H
#define SIMPLESCHEDULER_H

#include "solver.hpp"

namespace fms::solvers::SimpleScheduler {

[[nodiscard]] SolverOutput solve(problem::Instance &problemInstance,
                                 const cli::CLIArgs &args);
} // namespace fms::solvers::SimpleScheduler

#endif // SIMPLESCHEDULER_H
