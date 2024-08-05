/*
 * FORPFSSPSD.cpp
 *
 *  Created on: 22 May 2014
 *      Author: uwaqas
 *
 *  Modified by Joost van Pinxten (joost.vanpinxten@cpp.canon)
 */
#include "pch/containers.hpp"

#include "FORPFSSPSD/FORPFSSPSD.h"

#include "FORPFSSPSD/export_utilities.hpp"
#include "FORPFSSPSD/indices.hpp"
#include "FORPFSSPSD/operation.h"
#include "delay.h"
#include "delayGraph/delayGraph.h"
#include "delayGraph/export_utilities.h"
#include "partialsolution.h"

#include "pch/utils.hpp"
#include "utils/commandLine.h"

#include <cstddef>
#include <fmt/compile.h>
#include <fmt/ostream.h>
#include <sstream>
#include <tuple>
#include <utility>

using namespace DelayGraph;
using namespace FORPFSSPSD;

Plexity::Plexity(const std::string &s) {
    if (s == "D") {
        m_value = Value::DUPLEX;
    } else if (s == "S") {
        m_value = Value::SIMPLEX;
    } else {
        throw std::runtime_error("Invalid string for plexity");
    }
}

uint32_t Plexity::numberOfOps() const {
    switch (m_value) {
    case Value::DUPLEX:
        return 2;
    case Value::SIMPLEX:
        return 1;
    }
    
    throw std::runtime_error("Unhandled switch case for Plexity: " + std::to_string(m_value));
}

FORPFSSPSD::Instance::Instance(std::string problemName,
                               JobOperations jobs,
                               OperationMachineMap machineMapping,
                               DefaultOperationsTime processingTimes,
                               DefaultTimeBetweenOps setupTimes,
                               TimeBetweenOps setupTimesIndep,
                               TimeBetweenOps dueDates,
                               TimeBetweenOps dueDatesIndep,
                               JobsTime absoluteDueDates,
                               std::map<operation, unsigned int> sheetSizes,
                               delay defaultSheetSize,
                               delay maximumSheetSize,
                               ShopType shopType,
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
    m_defaultSheetSize(defaultSheetSize),
    m_maximumSheetSize(maximumSheetSize),
    m_shopType(shopType),
    m_outOfOrder(outOfOrder),
    m_problemName(std::move(problemName)) {

    computeJobsOutput();
    computeFlowVector();
}

void FORPFSSPSD::Instance::save(const PartialSolution &best, const commandLineArgs &args) const {
    // auto chosenEdges = best.getAllChosenEdges();
    auto chosenEdges = best.getChosenEdgesPerMachine();
    auto chosenASAPST = best.getASAPST();
    for(const auto& [m,mEdges] : chosenEdges) {
        for (const auto &i : mEdges) {
            LOG("final seq in best solution {}: {}-> {} ->{} with starts {} and {}",
                best.getId(),
                m_dg->get_vertex(i.src),
                i.weight,
                m_dg->get_vertex(i.dst),
                chosenASAPST[m_dg->get_vertex_id(i.src)],
                chosenASAPST[m_dg->get_vertex_id(i.dst)]);
        }
    }
    
    DelayGraph::Edges final_sequence;
    switch (args.algorithm) {
        default:
            final_sequence = createFinalSequence(best);
            break;
    }

    auto tempDg = *m_dg;
    auto [result, ASAPST] = algorithm::LongestPath::computeASAPST(tempDg, final_sequence);

    // for(const auto& edge : final_sequence){
    //     LOG(LOGGER_LEVEL::INFO,
    //         fmt::format("final sequence after rerun: {}-> {} ->{} with starts {} and {}", 
    //                             m_dg->get_vertex(edge.src), 
    //                             edge.weight,
    //                             m_dg->get_vertex(edge.dst),
    //                             ASAPST[m_dg->get_vertex_id(edge.src)],
    //                             ASAPST[m_dg->get_vertex_id(edge.dst)]));
    // }

    if (!result.positiveCycle.empty() || Logger::getLevel() >= LOGGER_LEVEL::DEBUG) {
        auto outName = fmt::format("output-graph_{}_{}", getProblemName(), best.getId());
        algorithm::LongestPath::dumpToFile(tempDg, ASAPST, fmt::format("{}-timings.txt", outName));
        DelayGraph::export_utilities::saveAsDot(
                *this, best, fmt::format("{}.dot", outName), result.positiveCycle);

        if (!result.positiveCycle.empty()) {
            throw FmsSchedulerException("Incorrect interleaving: did not converge");
        }
    }

    save(tempDg, args.outputFile, args.outputFormat, args.algorithm);

    // also save the sequence of operations for re-entrant machines for this solution
    // output: one machine per line

    std::ofstream file(args.outputFile + ".sequence");
    // output the first of the operations:
    for(const auto& [m,mEdges] : chosenEdges) {
        fmt::print(file, "{} ", m_dg->get_vertex(chosenEdges[m].at(0).src).operation);
        for (const auto &edge : chosenEdges[m]) {
            fmt::print(file, "{} ", m_dg->get_vertex(edge.dst).operation);
        }
        fmt::print(file, "\n");
    }

    file.close();
}


void FORPFSSPSD::Instance::save(delayGraph &graph,
                                std::string_view outputFile,
                                ScheduleOutputFormat outputFormat,
                                AlgorithmType algorithm) const {
    /* compute the ASAPST for the ordered vertices */
    auto [result, ASAPST] = algorithm::LongestPath::computeASAPST(graph);

    if(!result.positiveCycle.empty()) {
        DelayGraph::export_utilities::saveAsTikz(*this, graph, "incorrect.tex", result.positiveCycle);
        throw FmsSchedulerException("!! ASAPST did not converge when saving...");
    }

    // save the solution
    LOG("Saving the schedule to the output file.");
    std::ofstream file(fmt::format("{}.txt", outputFile));
    std::string maintFileOut(fmt::format("{}Maint.xml", outputFile));

    switch (outputFormat) {
    case ScheduleOutputFormat::OPERATION_TIMES:
        // loop and save the schedule
        for (const auto &[jobId, jobOps] : m_jobs) {
            bool first = true;
            for (auto jobOp : jobOps) {
                operation op{jobId, jobOp.operationId};
                const auto *comma = first ? "" : ",";
                first = false;

                if (graph.has_vertex(op)) {
                    const auto lastValue = ASAPST[graph.get_vertex_id(op)];
                    fmt::print(file, FMT_STRING("{}{:>15d}"), comma, lastValue);
                } else {
                    fmt::print(file, FMT_STRING("{}{:15}"), comma, "");
                }
            }
            // no more operations remaining
            file << '\n';
        }

        switch (algorithm) {
        case AlgorithmType::MIBHCS:
        case AlgorithmType::MISIM:
            ExportUtilities::saveAsXML(maintFileOut, *this, *m_dg);
        default:
            break;
        }
        break;
    case ScheduleOutputFormat::SEPARATION_AND_BUFFER:
        // separation time is the first operation's time (op 0)
        // buffer time is the time between the first and second re-entrant operation (op 1 and 2)
        for (const auto &[i, _] : m_jobs) {
            fmt::print(file,
                       FMT_STRING("{:>15d}\t{:>15d}\n"),
                       ASAPST[i.value * graph.get_vertex({i, 0}).id],
                       (ASAPST[graph.get_vertex({i, 2}).id] - ASAPST[graph.get_vertex({i, 1}).id]
                        - graph.get_edge(graph.get_vertex({i, 1}).id, graph.get_vertex({i, 2}).id)
                                  .weight));
        }
    default:
        break;
    }
    file.close();
    export_utilities::saveAsTikz(*this, graph, "out.tex");
}

nlohmann::json FORPFSSPSD::Instance::saveJSON(const PartialSolution &solution) const {
    nlohmann::json result{{"makespan", solution.getMakespan()}};
    for (const auto &e : solution.getAllChosenEdges()) {
        auto srcOp = m_dg->get_vertex(e.src).operation;
        auto dstOp = m_dg->get_vertex(e.dst).operation;
        result["schedule"] +=
                {{"edge",
                  {{srcOp.jobId.value, srcOp.operationId}, {dstOp.jobId.value, dstOp.operationId}}},
                 {"weight", e.weight}};
    }
    const auto &ASAPST = solution.getASAPST();
    for (const auto &v : m_dg->get_vertices()) {
        result["startTimes"] += {{"operation", {v.operation.jobId.value, v.operation.operationId}},
                                 {"time", ASAPST[v.id]}};
    }

    return result;
}

delay FORPFSSPSD::Instance::getSetupTime(operation op1, operation op2) const {
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
        return std::max(setup,*extra);
    }
    return setup;
}

delay FORPFSSPSD::Instance::getTrivialCompletionTimeLowerbound() {
    if (cachedTrivialLowerbound != std::numeric_limits<delay>::max()) {
        return cachedTrivialLowerbound;
    }

    delay first_pass_processing_time = 0;
    delay second_pass_processing_time = 0;
    std::optional<JobId> firstDuplex;
    for(unsigned int i = 0; i < getNumberOfJobs(); i++) {
        const JobId jobId(i);
        // FIXME: this is created specifically for CPP cases:

        if (!firstDuplex && getPlexity(jobId) == Plexity::DUPLEX) {
            firstDuplex = jobId;
        }

        if (firstDuplex) {
            if (m_dg->has_vertex({jobId, 1})) {
                first_pass_processing_time += getProcessingTime({jobId, 1});
            }
            second_pass_processing_time += getProcessingTime({jobId, 2});
            // TODO: also add half of the minimum incoming and outgoing setup times
        }
    }

    std::vector<delay> ASAPST = algorithm::LongestPath::initializeASAPST(*m_dg);
    algorithm::LongestPath::computeASAPST(*m_dg, ASAPST);

    delay firstDuplexStartTime = ASAPST.at(m_dg->get_source(MachineId(1)).id);
    if (firstDuplex) {
        firstDuplexStartTime = ASAPST[m_dg->get_vertex({*firstDuplex, 1}).id];
    }

    delay lastSheetUnloadTime = getSetupTime({m_jobsOutput.back(), 2}, {m_jobsOutput.back(), 3});

    delay lowerbound = firstDuplexStartTime
            + first_pass_processing_time
            + second_pass_processing_time
            + lastSheetUnloadTime;

    cachedTrivialLowerbound = std::max(lowerbound, ASAPST.back());
    return cachedTrivialLowerbound;
}

delay FORPFSSPSD::Instance::query(const vertex &vertex1,
                                  const vertex &vertex2) const {
    if (!DelayGraph::delayGraph::is_source(vertex1)) {
        return query(vertex1.operation, vertex2.operation);
    }

    // the operation is a 'virtual' (source) event, and does not have any weights associated with it.
    return 0;
}

delay FORPFSSPSD::Instance::query(const operation &src, const operation &dst) const {
    return getProcessingTime(src) + getSetupTime(src, dst);
}

std::optional<delay> FORPFSSPSD::Instance::queryDueDate(const operation &src,
                                                        const operation &dst) const {
    const auto result = m_dueDates.getMaybe(src, dst);
    const auto resultIndep = m_dueDatesIndep.getMaybe(src, dst);
    const auto extra = m_extraDueDates.getMaybe(src, dst);
    if(resultIndep){
        if (extra){
            return std::min(std::min(*result, *resultIndep),*extra);
        }
        return std::min(*result, *resultIndep);
    }
    return result;
}

DelayGraph::Edges
FORPFSSPSD::Instance::infer_pim_edges(const PartialSolution &ps) const {
    DelayGraph::Edges pimEdges;

    // start from the source job, this is by definition either a first pass duplex or a simplex print
    const MachineId &firstReEntrant = m_reEntrantMachines.front();
    const ReEntrantId &reEntrantId = findMachineReEntrantId(firstReEntrant);
    const OperationId &firstReEntrantOp = getMachineOperations(firstReEntrant).front();
    std::reference_wrapper<const DelayGraph::vertex> lastVertex =
            m_dg->get_source(m_machines.front());

    for (const auto &e : ps.getChosenEdges(firstReEntrant)) {
        const auto &dstOp = m_dg->get_vertex(e.dst).operation;
        const auto plexity = getPlexity(dstOp.jobId, reEntrantId);

        // Check if destination is part of the first re-entrant machine and it's not the second pass
        // of a duplex operation or its a maintenance operation
        if (dstOp.operationId != firstReEntrantOp && plexity == Plexity::DUPLEX
            || m_dg->is_maint(e.dst)) {
            continue;
        }

        const auto &dstV = m_dg->get_vertex(getJobOperations(dstOp.jobId).front());
        pimEdges.emplace_back(lastVertex.get().id, dstV.id, query(lastVertex, dstV));
        lastVertex = dstV;
    }

    return pimEdges;
}

[[nodiscard]] DelayGraph::Edges
FORPFSSPSD::Instance::createFinalSequence(const PartialSolution &ps) const {
    auto finalSequence = ps.getAllChosenEdges(); // Copy the chosen edges
    const auto &pimEdges = infer_pim_edges(ps);
    finalSequence.insert(finalSequence.end(), pimEdges.begin(), pimEdges.end());
    return finalSequence;
}

PartialSolution FORPFSSPSD::Instance::determine_partial_solution(std::vector<delay> ASAPST) const {

    DelayGraph::Edges edges;
    for (MachineId machine : m_machines) {
        if (m_operationsMappedOnMachine.at(machine).size() == 1) {
            continue;
        }
        std::map<OperationId, JobId> lastJobForOperation;
        for (auto op : m_operationsMappedOnMachine.at(machine)) {
            lastJobForOperation[op] = JobId(0);
        }

        bool done = false;
        const unsigned int maxIterations =
                m_operationsMappedOnMachine.at(machine).size() * m_jobs.size();
        unsigned int iteration = 0;
        JobId currentJob(-1);
        unsigned int currentOp = -1;
        bool first = true;
        while (!done) {
            if (iteration++ > maxIterations) {
                throw FmsSchedulerException(
                        fmt::format(FMT_COMPILE("Invariant violated: did not properly compute the "
                                                "order of operations on machine {}"),
                                    machine));
            }
            delay earliestNextOpTime = std::numeric_limits<delay>::max();
            JobId nextJob(std::numeric_limits<unsigned int>::max());
            unsigned int nextOp = std::numeric_limits<unsigned int>::max();
            for (auto op : m_operationsMappedOnMachine.at(machine)) {
                for (auto j = lastJobForOperation.at(op);
                     static_cast<std::uint32_t>(j) < getNumberOfJobs();
                     j++) {

                    if (m_dg->has_vertex({j, op})) {
                        // determine at what time this operation occurred
                        auto optime = ASAPST.at(m_dg->get_vertex({j, op}).id);

                        if (optime < earliestNextOpTime) {
                            earliestNextOpTime = optime;
                            nextJob = j;
                            nextOp = op;
                        }
                        break; // do not continue checking these jobs
                    }
                    lastJobForOperation[op] = j + 1;
                }
            }
            if (earliestNextOpTime == std::numeric_limits<delay>::max()) {
                throw FmsSchedulerException("Could not determine the next operation...");
            }

            if (nextJob == JobId::max() || nextOp == std::numeric_limits<unsigned int>::max()) {
                throw FmsSchedulerException("Could not determine the next job and operation...");
            }

            FORPFSSPSD::operation currentOperation{currentJob, currentOp};
            FORPFSSPSD::operation nextOperation{nextJob, nextOp};

            // if this is not the first operation found, we can add it as an edge for the partial
            // solution
            if (m_dg->has_vertex(currentOperation) && m_dg->has_vertex(nextOperation)) {
                const auto &src = m_dg->get_vertex(currentOperation);
                const auto &dst = m_dg->get_vertex(nextOperation);

                if (first) {
                    first = false;
                    edges.emplace_back(m_dg->get_source(machine).id, src.id, 0);
                }

                // construct an edge between these two vertices, as they occur directly after each
                // other
                edges.emplace_back(src.id, dst.id, query(src, dst));
            }

            // advance for that operation type to the next job:
            lastJobForOperation[nextOp] = nextJob + 1;

            currentJob = nextJob;
            currentOp = nextOp;

            done = true;
            for (auto op : m_operationsMappedOnMachine.at(machine)) {
                if (static_cast<std::uint32_t>(lastJobForOperation.at(op)) != getNumberOfJobs()) {
                    done = false;
                }
            }
        }
    }

    return PartialSolution({{m_reEntrantMachines.front(), edges}}, std::move(ASAPST));
}

PartialSolution Instance::loadSequence(std::istream &stream) const  {
    delayGraph graph = getDelayGraph();
    std::string line;
    
    DelayGraph::Edges sol_edges;
    while(std::getline(stream, line)) {
        std::istringstream line_stream(line);
        FORPFSSPSD::operation operation{JobId(0), 0};
        std::optional<std::reference_wrapper<DelayGraph::vertex>> prev;
        while(line_stream >> operation) {
            auto &current = graph.get_vertex(operation);
            if (prev.has_value()) {
                graph.add_edge(prev.value(), current, query(prev.value(), current));
                sol_edges.push_back(graph.get_edge(prev.value(), current));
            }
            prev = current;
        }
    }
    return PartialSolution({{m_reEntrantMachines.front(), sol_edges}},{});
}

void FORPFSSPSD::Instance::addExtraSetupTime(operation src, operation dst, delay value) {
    m_extraSetupTimes.insertMax(src, dst, value);

    // Update the edge in the delay graph. The query function already takes the minimum among the 
    // existing value and the passed one
    m_dg->add_edge(src, dst, query(src, dst));
}

void FORPFSSPSD::Instance::addExtraDueDate(operation src, operation dst, delay value) {
    m_extraDueDates.insertMin(src, dst, value);

    auto &srcV = m_dg->get_vertex(src);
    auto &dstV = m_dg->get_vertex(dst);

    if (m_dg->has_edge(srcV, dstV)) {
        const auto &edge = m_dg->get_edge(srcV, dstV);
        value = std::min(value, -edge.weight);
    }

    // Update the edge in the delay graph
    m_dg->add_edge(srcV, dstV, -value);
}

FORPFSSPSD::Plexity FORPFSSPSD::Instance::getPlexity(JobId jobId, ReEntrantId reentrancy) const {
    // TODO: Using an enum for plexity doesn't support n-re-entrancy
    auto it = m_jobPlexity.find(jobId);
    if (it != m_jobPlexity.end()) {
        auto plexity = it->second.at(static_cast<std::size_t>(reentrancy.value));
        if (plexity == static_cast<ReEntrancies>(Plexity::SIMPLEX)) {
            return Plexity::SIMPLEX;
        }
    } // The XML format allows not specifying the flow vector so by default we use duplex
    return Plexity::DUPLEX;
}

FORPFSSPSD::ReEntrancies FORPFSSPSD::Instance::getJobReEntrancies(JobId jobId,
                                                                  ReEntrantId reentrancy) const {
    const auto it = m_jobPlexity.find(jobId);
    if (it != m_jobPlexity.end()) {
        return it->second.at(static_cast<std::size_t>(reentrancy.value));
    }

    // By default we always have as many reentrancies as the number of operations per machine
    const auto mId = getReEntrantMachineId(reentrancy);
    return ReEntrancies(m_operationsMappedOnMachine.at(mId).size());
}

ReEntrancies FORPFSSPSD::Instance::getReEntrancies(const operation &op) const {
    const auto it = m_reEntrantMachineToId.find(getMachine(op));

    // Operation belongs to a reentrant machine
    if (it != m_reEntrantMachineToId.end()) {
        return getJobReEntrancies(op.jobId, it->second);
    }

    return ReEntrancies(1);
}

std::unordered_set<unsigned int>
FORPFSSPSD::Instance::getUniqueSheetSizes(std::size_t startJob) const {
    std::unordered_set<unsigned int> uniqueSheetSizes;
    for (std::size_t i = startJob; i < m_jobsOutput.size(); i++) {
        auto sheetSize = getSheetSize({m_jobsOutput[i], 0});
        uniqueSheetSizes.insert(sheetSize);
    }
    return uniqueSheetSizes;
}

delay FORPFSSPSD::Instance::getSheetSize(operation op) const {
    auto it = m_sheetSizes.find(op);
    if (it == m_sheetSizes.end()) {
        LOG(LOGGER_LEVEL::DEBUG, fmt::format("Sheet size for {} not specified!", op));
        return m_defaultSheetSize;
    }
    return it->second;
}

void FORPFSSPSD::Instance::computeFlowVector() {
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

void FORPFSSPSD::Instance::computeJobsOutput() {
    for (const auto &[jobId, _] : m_jobs) {
        m_jobsOutput.push_back(jobId);
    }

    // Output of jobs is defined by Id
    std::sort(m_jobsOutput.begin(), m_jobsOutput.end());

    // Store output positions
    for (std::size_t i = 0; i < m_jobsOutput.size(); ++i) {
        m_jobToOutputPosition.emplace(m_jobsOutput.at(i), i);
    }
}

