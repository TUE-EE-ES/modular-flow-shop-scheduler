#ifndef NOFIXEDORDERSOLUTION
#define NOFIXEDORDERSOLUTION

#include "FORPFSSPSD/indices.hpp"
#include "delayGraph/delayGraph.h"
#include "partialsolution.h"

#include <utility>
#include <vector>

class NoFixedOrderSolution {
public:
    PartialSolution solution;
    std::vector<FORPFSSPSD::JobId> jobOrder;

    // Default constructor
    NoFixedOrderSolution() : solution({}, {}) {}

    // parameterised constructor
    template <typename... Args>
    explicit NoFixedOrderSolution(std::vector<FORPFSSPSD::JobId> jobOrder, Args &&...args) :
        solution(std::forward<Args>(args)...), jobOrder(std::move(jobOrder)) {}

    NoFixedOrderSolution(std::vector<FORPFSSPSD::JobId> jobOrder, PartialSolution solution) :
        solution(std::move(solution)), jobOrder(std::move(jobOrder)) {}

    const DelayGraph::delayGraph &getDelayGraph() const { return m_dg; }
    void updateDelayGraph(const DelayGraph::delayGraph &newGraph) { m_dg = newGraph; }

private:
    DelayGraph::delayGraph m_dg;

};

#endif // NOFIXEDORDERSOLUTION