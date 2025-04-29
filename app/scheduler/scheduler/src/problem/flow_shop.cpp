/*
 * FORPFSSPSD.cpp
 *
 *  Created on: 22 May 2014
 *      Author: uwaqas
 *
 *  Modified by Joost van Pinxten (joost.vanpinxten@cpp.canon)
 */
#include "fms/pch/containers.hpp"
#include "fms/pch/utils.hpp"
#include "fms/pch/fmt.hpp"

#include "fms/problem/flow_shop.hpp"

#include "fms/cg/constraint_graph.hpp"
#include "fms/delay.hpp"
#include "fms/problem/operation.hpp"
#include "fms/solvers/partial_solution.hpp"

#include "fms/cli/command_line.hpp"

#include <cstddef>
#include <utility>

namespace fms::problem {

Instance::Instance(std::string problemName,
                   JobOperations jobs,
                   OperationMachineMap machineMapping,
                   DefaultOperationsTime processingTimes,
                   DefaultTimeBetweenOps setupTimes,
                   TimeBetweenOps setupTimesIndep,
                   TimeBetweenOps dueDates,
                   TimeBetweenOps dueDatesIndep,
                   JobsTime absoluteDueDates,
                   OperationSizes sheetSizes,
                   delay maximumSheetSize,
                   cli::ShopType shopType,
                   bool outOfOrder) :
    m_jobs(std::move(jobs)),
    m_machineMapping(std::move(machineMapping)),
    m_processingTimes(std::move(processingTimes)),
    m_setupTimes(std::move(setupTimes)),
    m_setupTimesIndep(std::move(setupTimesIndep)),
    m_dueDates(std::move(dueDates)),
    m_dueDatesIndep(std::move(dueDatesIndep)),
    m_absoluteDueDates(std::move(absoluteDueDates)),
    m_sheetSizes(std::move(sheetSizes)),
    m_maximumSheetSize(maximumSheetSize),
    m_shopType(shopType),
    m_outOfOrder(outOfOrder),
    m_problemName(std::move(problemName)) {

    for (const auto &[jobId, ops] : m_jobs) {
        for (const auto &op : ops) {
            m_jobToMachineOps[jobId][getMachine(op)].push_back(op);
        }
    }

    computeJobsOutput();
    computeFlowVector();
}

delay Instance::getSetupTime(Operation op1, Operation op2) const {
    // It is possible that op1 or op2 are source/sink operations. In which case the sequence-
    // dependent setup time is not relevant.
    delay setup = 0;
    if (isValid(op1) && isValid(op2)) {
        const auto m1 = getMachine(op1);
        const auto m2 = getMachine(op2);

        if (m1 == m2) {
            setup = m_setupTimes(op1, op2);
        }
    }

    const auto timeIndep = m_setupTimesIndep.getMaybe(op1, op2);
    if (timeIndep) {
        setup = std::max(setup, *timeIndep);
    }

    const auto extra = m_extraSetupTimes.getMaybe(op1, op2);
    if (extra) {
        return std::max(setup, *extra);
    }
    return setup;
}

delay Instance::query(const cg::Vertex &vertex1, const cg::Vertex &vertex2) const {
    if (!cg::ConstraintGraph::isSource(vertex1)) {
        return query(vertex1.operation, vertex2.operation);
    }

    // the operation is a 'virtual' (source) event, and does not have any weights associated with
    // it.
    return 0;
}

delay Instance::query(const Operation &src, const Operation &dst) const {
    if (src.isMaintenance()) {
        return m_maintPolicy.getMaintDuration(src);
    }
    return getProcessingTime(src) + getSetupTime(src, dst);
}

std::optional<delay> Instance::queryDueDate(const Operation &src, const Operation &dst) const {
    const auto result = m_dueDates.getMaybe(src, dst);
    const auto resultIndep = m_dueDatesIndep.getMaybe(src, dst);
    const auto extra = m_extraDueDates.getMaybe(src, dst);
    if (resultIndep) {
        if (extra) {
            return std::min(std::min(*result, *resultIndep), *extra);
        }
        return std::min(*result, *resultIndep);
    }
    return result;
}

// [[nodiscard]] cg::Edges Instance::createFinalSequence(const solvers::PartialSolution &ps) const {
//     auto finalSequence = ps.getAllChosenEdges(); // Copy the chosen edges
//     const auto &pimEdges = infer_pim_edges(ps);
//     finalSequence.insert(finalSequence.end(), pimEdges.begin(), pimEdges.end());
//     return finalSequence;
// }

void Instance::addExtraSetupTime(Operation src, Operation dst, delay value) {
    m_extraSetupTimes.insertMax(src, dst, value);

    // Update the edge in the delay graph. The query function already takes the minimum among the
    // existing value and the passed one
    m_dg->addEdge(src, dst, query(src, dst));
}

void Instance::addExtraDueDate(Operation src, Operation dst, delay value) {
    m_extraDueDates.insertMin(src, dst, value);

    auto &srcV = m_dg->getVertex(src);
    auto &dstV = m_dg->getVertex(dst);

    if (m_dg->hasEdge(srcV, dstV)) {
        const auto &edge = m_dg->getEdge(srcV, dstV);
        value = std::min(value, -edge.weight);
    }

    // Update the edge in the delay graph
    m_dg->addEdge(srcV, dstV, -value);
}

OperationsVector Instance::getJobOperationsOnMachine(JobId jobId, MachineId machineId) const {
    const auto it = m_jobToMachineOps.find(jobId);
    if (it == m_jobToMachineOps.end()) {
        return {};
    }

    const auto it2 = it->second.find(machineId);
    if (it2 == it->second.end()) {
        return {};
    }

    return it2->second;
}

ReEntrancies Instance::getReEntrancies(JobId jobId, ReEntrantId reentrancy) const {
    const auto it = m_jobPlexity.find(jobId);
    if (it != m_jobPlexity.end()) {
        return it->second.at(static_cast<std::size_t>(reentrancy.value));
    }

    // By default we always have as many reentrancies as the number of operations per machine
    const auto mId = getReEntrantMachineId(reentrancy);
    return ReEntrancies(m_operationsMappedOnMachine.at(mId).size());
}

ReEntrancies Instance::getReEntrancies(const Operation &op) const {
    const auto it = m_reEntrantMachineToId.find(getMachine(op));

    // Operation belongs to a reentrant machine
    if (it != m_reEntrantMachineToId.end()) {
        return getReEntrancies(op.jobId, it->second);
    }

    return ReEntrancies{1};
}

std::unordered_set<unsigned int> Instance::getUniqueSheetSizes(std::size_t startJob) const {
    std::unordered_set<unsigned int> uniqueSheetSizes;
    for (std::size_t i = startJob; i < m_jobsOutput.size(); i++) {
        auto sheetSize = getSheetSize({m_jobsOutput[i], 0});
        uniqueSheetSizes.insert(sheetSize);
    }
    return uniqueSheetSizes;
}

void Instance::computeFlowVector() {
    m_flowVector.clear();
    m_machines.clear();
    m_machineToIndex.clear();
    m_operationsMappedOnMachine.clear();

    // Find operationId per machine and flow of machines
    std::unordered_set<MachineId> machines;
    std::unordered_map<MachineId, std::unordered_set<OperationId>> operationsMappedOnMachine;
    for (const auto &[jobId, jobOps] : m_jobs) {
        for (const auto &op : jobOps) {
            const auto machine = getMachine(op);
            operationsMappedOnMachine[machine].insert(op.operationId);

            if (machines.find(machine) == machines.end()) {
                m_machines.push_back(machine);
                machines.insert(machine);
            }
        }
    }

    // Construct machine flow vector, operations mapped on machine and find re-entrant machines
    for (std::size_t i = 0; i < m_machines.size(); ++i) {
        const auto machine = m_machines[i];
        const auto &ops = operationsMappedOnMachine.at(machine);

        m_machineToIndex.emplace(machine, i);
        OperationFlowVector machineOrderedOps(ops.begin(), ops.end());
        std::sort(machineOrderedOps.begin(), machineOrderedOps.end());
        m_flowVector.insert(m_flowVector.end(), machineOrderedOps.begin(), machineOrderedOps.end());
        m_operationsMappedOnMachine[machine] = std::move(machineOrderedOps);

        for (const auto op : ops) {
            m_operationToMachine.emplace(op, machine);
        }

        if (ops.size() > 1) {
            const auto reEntrantId = static_cast<ReEntrantId>(m_reEntrantMachineToId.size());
            m_reEntrantMachineToId[machine] = reEntrantId;
            m_reEntrantMachines.push_back(machine);
        }
    }

    // Now that we know which ones are re-entrant machines we can find the jobs plexity
    for (const auto &[jobId, jobOps] : m_jobs) {
        std::vector<ReEntrancies> jobReEntrancies(m_reEntrantMachines.size(), ReEntrancies(0));
        for (const auto &op : jobOps) {
            const auto machine = getMachine(op);
            const auto it = m_reEntrantMachineToId.find(machine);
            if (it == m_reEntrantMachineToId.end()) {
                continue;
            }
            ++jobReEntrancies[static_cast<std::size_t>(it->second.value)];
        }

        if (jobReEntrancies.empty()) {
            continue;
        }
        m_jobPlexity[jobId] = jobReEntrancies;
    }
}

void Instance::computeJobsOutput() {
    auto jobIds = m_jobs | std::views::keys;
    m_jobsOutput.assign(jobIds.begin(), jobIds.end());
    std::ranges::sort(m_jobsOutput);

    // Store output positions
    for (std::size_t i = 0; i < m_jobsOutput.size(); ++i) {
        m_jobToOutputPosition.emplace(m_jobsOutput.at(i), i);
    }
}

} // namespace fms::problem
