#ifndef SIMPLESCHEDULER_H
#define SIMPLESCHEDULER_H

#include "solver.h"

namespace algorithm::SimpleScheduler {

[[nodiscard]] SolverOutput solve(FORPFSSPSD::Instance &problemInstance,
                                 const commandLineArgs &args);
} // namespace algorithm::SimpleScheduler

#endif // SIMPLESCHEDULER_H
