#include "fms/pch/containers.hpp"
#include "fms/pch/fmt.hpp"
#include "fms/pch/utils.hpp"

#include "fms/solvers/partial_solution.hpp"

#include "fms/problem/flow_shop.hpp"
#include "fms/solvers/utils.hpp"

namespace fms::solvers {

cg::Edges PartialSolution::getChosenEdges(problem::MachineId machineId,
                                          const problem::Instance &problem) const {
    const auto &ops = m_chosenSequences.at(machineId);
    return SolversUtils::getEdgesFromSequence(problem, ops, machineId);
}

problem::OperationsVector::const_iterator
PartialSolution::firstPossibleOp(problem::MachineId machineId) const {
    using vec_t = decltype(m_chosenSequences)::value_type::second_type;
    auto itChosen = m_chosenSequences.find(machineId);
    if (itChosen == m_chosenSequences.end()) {
        itChosen = std::get<0>(m_chosenSequences.emplace(machineId, vec_t()));
    }

    auto itFirst = m_firstFeasibleEdge.find(machineId);
    if (itFirst == m_firstFeasibleEdge.end()) {
        itFirst = std::get<0>(m_firstFeasibleEdge.emplace(machineId, 0));
    }
    return itChosen->second.begin() + static_cast<vec_t::difference_type>(itFirst->second);
}

problem::OperationsVector::const_iterator
PartialSolution::firstMaintOp(problem::MachineId machineId) const {
    using vec_t = decltype(m_chosenSequences)::mapped_type;
    auto itChosen = m_chosenSequences.find(machineId);
    if (itChosen == m_chosenSequences.end()) {
        itChosen = std::get<0>(m_chosenSequences.emplace(machineId, vec_t()));
    }

    auto itFirst = m_firstMaintEdge.find(machineId);
    if (itFirst == m_firstMaintEdge.end()) {
        itFirst = std::get<0>(m_firstMaintEdge.emplace(machineId, 0));
    }
    return itChosen->second.begin() + static_cast<vec_t::difference_type>(itFirst->second);
}

problem::OperationsVector::const_iterator
PartialSolution::latestOp(problem::MachineId machineId) const {
    using vec_t = decltype(m_chosenSequences)::mapped_type;
    auto itChosen = m_chosenSequences.find(machineId);
    if (itChosen == m_chosenSequences.end()) {
        itChosen = std::get<0>(m_chosenSequences.emplace(machineId, vec_t()));
    }

    auto itLast = m_lastInsertedEdge.find(machineId);
    if (itLast == m_lastInsertedEdge.end()) {
        itLast = std::get<0>(m_lastInsertedEdge.emplace(machineId, 0));
    }
    return itChosen->second.begin() + static_cast<vec_t::difference_type>(itLast->second);
}

PartialSolution PartialSolution::add(problem::MachineId machineId,
                                     const SchedulingOption &c,
                                     const std::vector<delay> &ASAPST) const {

    auto newSequences = m_chosenSequences;
    auto &machineSequence = newSequences[machineId];

    // insert the previous edge before the insertion position
    machineSequence.insert(machineSequence.begin() + static_cast<SequenceDiff>(c.position), c.curO);

    // any solution can only start from the next position
    // but adding a maintenance should not change the first feasible edge
    auto newLastEdges = m_lastInsertedEdge;
    newLastEdges[machineId] = c.position + 1;

    auto newFirstMaintEdges = m_firstMaintEdge;

    auto newFirstFeasibleEdges = m_firstFeasibleEdge;
    newFirstFeasibleEdges[machineId] =
            (c.is_maint) ? newFirstFeasibleEdges[machineId] + 1 : c.position + 1;

    PartialSolution ps(std::move(newSequences),
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

PartialSolution PartialSolution::remove(problem::MachineId machineId,
                                        const SchedulingOption &c,
                                        const std::vector<delay> &ASAPST,
                                        bool after) const {

    auto newEdges = m_chosenSequences;
    auto &machineEdges = newEdges[machineId];

    // overwrite the removal position's edge with the curr edge
    machineEdges.erase(machineEdges.begin() + static_cast<SequenceDiff>(c.position));

    // removal should only change last inserted edge if removing from before that edge
    // only currently correct because repair starts removing only after last inserted edge
    auto newLastEdges = m_lastInsertedEdge;
    if (!after) {
        newLastEdges[machineId] = newLastEdges[machineId] - 1;
        LOG_I(FMT_COMPILE("new last edge is {}\n"), newLastEdges[machineId]);
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

cg::Edges PartialSolution::getAllChosenEdges(const problem::Instance &problem) const {
    return SolversUtils::getAllEdgessFromSequences(problem, m_chosenSequences);
}

Sequence PartialSolution::getInferredInputSequence(const problem::Instance &problem) const {
    return SolversUtils::getInferredInputSequence(problem, m_chosenSequences);
}

void PartialSolution::addInferredInputSequence(const problem::Instance &problem) {
    auto seq = getInferredInputSequence(problem);
    setMachineSequence(problem.getMachines().front(), seq);
}

cg::Edges PartialSolution::getAllAndInferredEdges(const problem::Instance &problem) const {
    return SolversUtils::getAllEdgesPlusInferredEdges(problem, m_chosenSequences);
}

delay PartialSolution::getRealMakespan(const problem::Instance &problem) const {
    // Find the last operation in the system
    const auto jobLast = problem.getJobsOutput().back();
    const auto lastOp = problem.jobs(jobLast).back();

    const auto vId = problem.getDelayGraph().getVertex(lastOp).id;
    return ASAPST.at(vId) + problem.processingTimes(lastOp);
}

std::string chosenSequencesToString(const PartialSolution &solution) {
    fmt::memory_buffer out;

    out.append(std::string_view{"{"});
    for (const auto &[machineId, ops] : solution.getChosenSequencesPerMachine()) {
        fmt::format_to(
                std::back_inserter(out), FMT_COMPILE("{}: [{}]\n"), machineId, fmt::join(ops, "->"));
    }
    out.append(std::string_view{"}"});
    return fmt::to_string(out);
}

} // namespace fms::solvers
