#ifndef MNEH_H
#define MNEH_H

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "FORPFSSPSD/indices.hpp"
#include "FORPFSSPSD/module.hpp"
#include "longest_path.h"

#include "pch/containers.hpp"

#include <memory>

namespace algorithm {

class MNEH
{
    static DelayGraph::Edges createInitialSequence(const FORPFSSPSD::Instance &problemInstance,
                                                   FORPFSSPSD::MachineId reEntrantMachine);

    static DelayGraph::Edges chooseSequence(const FORPFSSPSD::Instance &problemInstance,
                                            FORPFSSPSD::MachineId reEntrantMachine,
                                            const DelayGraph::Edges &seedSequence,
                                            DelayGraph::delayGraph &dg,
                                            const commandLineArgs &args);

    static std::tuple<DelayGraph::Edges, PartialSolution>
    updateSequence(const FORPFSSPSD::Instance &problemInstance,
                   FORPFSSPSD::MachineId reEntrantMachine,
                   const DelayGraph::Edges &startingSequence,
                   DelayGraph::delayGraph &dg,
                   const commandLineArgs &args);

    static bool validateSequence(const FORPFSSPSD::Instance &problemInstance,
                                 const DelayGraph::Edges &sequence,
                                 FORPFSSPSD::MachineId reEntrantMachine,
                                 const DelayGraph::delayGraph &dg);

public:
    static PartialSolution solve(FORPFSSPSD::Instance &problemInstance, const commandLineArgs &args);

};
}// namespace algorithm

#endif // MNEH_H