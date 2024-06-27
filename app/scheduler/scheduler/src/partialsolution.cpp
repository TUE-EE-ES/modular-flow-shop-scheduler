#include "pch/containers.hpp"
#include "pch/utils.hpp"

#include "partialsolution.h"

#include "FORPFSSPSD/FORPFSSPSD.h"

#include <fmt/ostream.h>
#include <ranges>

DelayGraph::Edges::const_iterator
PartialSolution::first_possible_edge(FORPFSSPSD::MachineId machineId) const {
    using vec_t = decltype(m_chosenEdges)::value_type::second_type;
    auto itChosen = m_chosenEdges.find(machineId);
    if (itChosen == m_chosenEdges.end()) {
        itChosen = std::get<0>(m_chosenEdges.emplace(machineId, vec_t()));
    }

    auto itFirst = m_firstFeasibleEdge.find(machineId);
    if (itFirst == m_firstFeasibleEdge.end()) {
        itFirst = std::get<0>(m_firstFeasibleEdge.emplace(machineId, 0));
    }
    return itChosen->second.begin() + static_cast<vec_t::difference_type>(itFirst->second);
}

DelayGraph::Edges::const_iterator
PartialSolution::first_maint_edge(FORPFSSPSD::MachineId machineId) const {
    using vec_t = decltype(m_chosenEdges)::value_type::second_type;
    auto itChosen = m_chosenEdges.find(machineId);
    if (itChosen == m_chosenEdges.end()) {
        itChosen = std::get<0>(m_chosenEdges.emplace(machineId, vec_t()));
    }

    auto itFirst = m_firstMaintEdge.find(machineId);
    if (itFirst == m_firstMaintEdge.end()) {
        itFirst = std::get<0>(m_firstMaintEdge.emplace(machineId, 0));
    }
    return itChosen->second.begin() + static_cast<vec_t::difference_type>(itFirst->second);
}

DelayGraph::Edges::const_iterator
PartialSolution::latest_edge(FORPFSSPSD::MachineId machineId) const {
    using vec_t = decltype(m_chosenEdges)::value_type::second_type;
    auto itChosen = m_chosenEdges.find(machineId);
    if (itChosen == m_chosenEdges.end()) {
        itChosen = std::get<0>(m_chosenEdges.emplace(machineId, vec_t()));
    }

    auto itLast = m_lastInsertedEdge.find(machineId);
    if (itLast == m_lastInsertedEdge.end()) {
        itLast = std::get<0>(m_lastInsertedEdge.emplace(machineId, 0));
    }
    return itChosen->second.begin() + static_cast<vec_t::difference_type>(itLast->second);
}

std::string chosenEdgesToString(const PartialSolution &solution, const DelayGraph::delayGraph &dg) {
    fmt::memory_buffer out;

    fmt::format_to(std::back_inserter(out), FMT_COMPILE("{{"));
    for (const auto &[machineId, edges] : solution.getChosenEdgesPerMachine()) {
        fmt::format_to(std::back_inserter(out), FMT_COMPILE("{}: ["), machineId);
        for (const auto &e : edges) {
            const auto &op = dg.get_vertex(e.src).operation;
            fmt::format_to(std::back_inserter(out), FMT_COMPILE("{}->"), op);
        }
        const auto &lastOp = dg.get_vertex(edges.back().dst).operation;
        fmt::format_to(std::back_inserter(out), FMT_COMPILE("{}]"), lastOp);
    }
    fmt::format_to(std::back_inserter(out), FMT_COMPILE("}}"));
    return fmt::to_string(out);
}

PartialSolution PartialSolution::add(FORPFSSPSD::MachineId machineId,
                                     const algorithm::option &c,
                                     const std::vector<delay> &ASAPST) const {

    auto newEdges = m_chosenEdges;
    auto &machineEdges = newEdges[machineId];

    // insert the previous edge before the insertion position
    machineEdges.insert(machineEdges.begin() + c.position, c.prevE);
    // overwrite the insertion position's edge with the next edge
    machineEdges[c.position + 1] = c.nextE;

    // any solution can only start from the next position
    // but adding a maintenance should not change the first feasible edge
    auto newLastEdges = m_lastInsertedEdge;
    newLastEdges[machineId] = c.position + 1;

    auto newFirstMaintEdges = m_firstMaintEdge;

    auto newFirstFeasibleEdges = m_firstFeasibleEdge;
    newFirstFeasibleEdges[machineId] = (c.is_maint) ? newFirstFeasibleEdges[machineId] + 1 : c.position + 1;

    PartialSolution ps(std::move(newEdges),
                       ASAPST,
                       std::move(newLastEdges),
                       std::move(newFirstFeasibleEdges),
                       std::move(newFirstMaintEdges));
    ps.prev_id = id;
    ps.setMaintCount(maintCount);
    ps.setRepairCount(repairCount);
    ps.setReprintCount(reprintCount);
    return ps;
}

PartialSolution PartialSolution::remove(FORPFSSPSD::MachineId machineId,
                                        const algorithm::option &c,
                                        const std::vector<delay> &ASAPST,
                                        bool after) const {

    auto newEdges = m_chosenEdges;
    auto &machineEdges = newEdges[machineId];

    // overwrite the removal position's edge with the curr edge
    machineEdges.erase(machineEdges.begin() + c.position);
    machineEdges[c.position-1] = c.prevE;

    //removal should only change last inserted edge if removing from before that edge
    //only currently correct because repair starts removing only after last inserted edge
    auto newLastEdges = m_lastInsertedEdge;
    if (!after){
        newLastEdges[machineId] = newLastEdges[machineId] - 1;
        LOG(LOGGER_LEVEL::INFO,
            fmt::format(FMT_COMPILE("new last edge is {}\n"), newLastEdges[machineId]));
    }

    auto newFirstMaintEdges = m_firstMaintEdge;

    auto newFirstFeasibleEdges = m_firstFeasibleEdge;
    newFirstFeasibleEdges[machineId] = newFirstFeasibleEdges[machineId] - 1;

    PartialSolution ps(std::move(newEdges),
                       ASAPST,
                       std::move(newLastEdges),
                       std::move(newFirstFeasibleEdges),
                       std::move(newFirstMaintEdges));
    ps.prev_id = id;
    ps.setMaintCount(maintCount);
    ps.setRepairCount(repairCount);
    ps.setReprintCount(reprintCount);
    return ps;
}

DelayGraph::Edges PartialSolution::getAllChosenEdges() const {
    auto view = m_chosenEdges | std::views::values | std::views::join;
    return {view.begin(), view.end()};
}

delay PartialSolution::getRealMakespan(const FORPFSSPSD::Instance& problem) const {
    // Find the last operation in the system
    const auto jobLast = problem.getJobsOutput().back();
    const auto lastOp = problem.jobs(jobLast).back();

    const auto vId = problem.getDelayGraph().get_vertex(lastOp).id;
    return ASAPST.at(vId) + problem.processingTimes(lastOp);
}
