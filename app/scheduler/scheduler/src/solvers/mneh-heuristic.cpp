#include "pch/containers.hpp"

#include "solvers/mneh-heuristic.h"

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "FORPFSSPSD/indices.hpp"
#include "FORPFSSPSD/operation.h"
#include "delayGraph/builder.hpp"
#include "delayGraph/delayGraph.h"
#include "delayGraph/edge.h"
#include "longest_path.h"
#include "maintenanceheuristic.h"
#include "solvers/utils.hpp"

#include <fmt/chrono.h>
#include <fmt/compile.h>

using namespace algorithm;
using namespace std;
using namespace FORPFSSPSD;
using namespace DelayGraph;

PartialSolution MNEH::solve(FORPFSSPSD::Instance &problemInstance, const commandLineArgs &args){
    // solve the instance
    LOG("Computation of the schedule started");
    
    // make a copy of the delaygraph
    if (!problemInstance.isGraphInitialized()) {
        problemInstance.updateDelayGraph(Builder::FORPFSSPSD(problemInstance));
    }
    delayGraph dg = problemInstance.getDelayGraph();

    // We only support a single re-entrant machine in the system so choose the first one
    MachineId reentrant_machine = problemInstance.getReEntrantMachines().front();
    if (problemInstance.getMachineOperations(reentrant_machine).size() > 2) {
        throw std::runtime_error("Multiple re-entrancies not implemented yet");
    }

    auto [result, ASAPST] = SolversUtils::checkSolutionAndOutputIfFails(problemInstance);
    auto initial_sequence = MNEH::createInitialSequence(problemInstance,reentrant_machine);
    if(args.verbose >= LOGGER_LEVEL::DEBUG) {
        LOG(LOGGER_LEVEL::DEBUG, fmt::format(FMT_COMPILE("Initial sequence chosen")));
        for (auto e: initial_sequence){
            LOG(LOGGER_LEVEL::DEBUG,
                fmt::format(FMT_COMPILE("{} {} {}"),
                            dg.get_vertex(e.src).operation,
                            e,
                            dg.get_vertex(e.dst).operation));
        }
    }

    auto chosen_sequence = MNEH::chooseSequence(problemInstance,reentrant_machine,initial_sequence,dg,args);
    auto [chosen_result, chosen_ASAPST] = algorithm::LongestPath::computeASAPST(dg, chosen_sequence);

    PartialSolution solution({{reentrant_machine, chosen_sequence}}, chosen_ASAPST);

    switch (args.algorithm){
        case AlgorithmType::MINEH:
        case AlgorithmType::MINEHSIM:
            LOG("final maint check");
            for (auto edge: solution.getAllChosenEdges()){
                LOG("before: {}->{}",dg.get_vertex(edge.src), dg.get_vertex(edge.dst));
            }
            std::tie(solution,dg) = MaintenanceHeuristic::triggerMaintenance(dg, problemInstance, reentrant_machine, solution, args);
            problemInstance.updateDelayGraph(dg);
            for (auto edge: solution.getAllChosenEdges()){
                LOG("after: {}->{}",dg.get_vertex(edge.src), dg.get_vertex(edge.dst));
            }
            break;
        default:
            break;
    }
    // for (auto edge: solution.getAllChosenEdges()){
    //     LOG("after neh seq: {}->{}",dg.get_vertex(edge.src), dg.get_vertex(edge.dst));
    // }


    return solution;
}

DelayGraph::Edges MNEH::createInitialSequence(const FORPFSSPSD::Instance &problemInstance,
                                              MachineId reEntrantMachine) {

    DelayGraph::Edges initialSequence;
    const delayGraph & dg = problemInstance.getDelayGraph();
    ReEntrantId reEntrantMachineId = problemInstance.findMachineReEntrantId(reEntrantMachine);

    // check how many operations are mapped
    const std::vector<OperationId> &ops = problemInstance.getMachineOperations(reEntrantMachine);

    /// no ordering is required if the number of operations mapped is 1
    if (ops.size() <= 1) {
        throw std::runtime_error(fmt::format("Machine {} is not re-entrant", reEntrantMachine));
    }

    std::optional<JobId> lastDuplexJob;

    // Add all first passes (of _duplex_ jobs) directly followed by their second passes to the
    // initial sequence
    for (auto job : problemInstance.getJobsOutput()) { // For all but the last job
        if (problemInstance.getPlexity(job, reEntrantMachineId) == FORPFSSPSD::Plexity::DUPLEX) {
            const auto &vTo = dg.get_vertex({job, ops[0]});
            const auto &vToSecond = dg.get_vertex({job, ops[1]});
            std::optional<edge> edgeFirst, edgeSecond;

            // True on first iteration
            if (!lastDuplexJob.has_value()) {
                const auto &vFrom = dg.get_source(problemInstance.getMachine(ops[0]));
                edgeFirst = edge(vFrom.id, vTo.id, 0);
                edgeSecond = edge(vTo.id, vToSecond.id, problemInstance.query(vTo, vToSecond));
            } else {
                const auto &vFrom = dg.get_vertex({lastDuplexJob.value(), ops[1]});
                edgeFirst = edge(vFrom.id, vTo.id, problemInstance.query(vFrom, vTo));
                edgeSecond = edge(vTo.id, vToSecond.id, problemInstance.query(vTo, vToSecond));
            }

            initialSequence.push_back(*edgeFirst);
            initialSequence.push_back(*edgeSecond);
            lastDuplexJob = job;
        }
    }

    if (!lastDuplexJob.has_value()) {
        throw FmsSchedulerException("Nothing to schedule; only simplex sheets!");
    }

    return initialSequence;
}

DelayGraph::Edges MNEH::chooseSequence(const FORPFSSPSD::Instance &problemInstance,
                                       MachineId reEntrantMachine,
                                       const DelayGraph::Edges &seedSequence,
                                       delayGraph &dg,
                                       const commandLineArgs &args) {
    // seed sequence must be valid
    auto ASAPST = algorithm::LongestPath::initializeASAPST(dg);
    PartialSolution seedSolution({{reEntrantMachine, seedSequence}}, ASAPST);
    auto final_sequence = problemInstance.createFinalSequence(seedSolution);
    auto [result, ASAPSTr] = algorithm::LongestPath::computeASAPST(dg, final_sequence);
    seedSolution.setASAPST(ASAPSTr);

    if (!result.positiveCycle.empty()){
        throw FmsSchedulerException("seed sequence infeasible \n");
    }
    
    auto [builtSequence, builtSolution] = updateSequence(problemInstance, reEntrantMachine, seedSequence, dg, args);

    switch (args.algorithm) {
        case AlgorithmType::MINEH: {
            auto [builtMaintSolution, builtMaintDg] =
                    MaintenanceHeuristic::triggerMaintenance(dg, problemInstance, reEntrantMachine, builtSolution, args);
            builtSolution = builtMaintSolution;
            auto [seedMaintSolution, seedMaintDg] =
                    MaintenanceHeuristic::triggerMaintenance(dg, problemInstance, reEntrantMachine, seedSolution, args);
            seedSolution = seedMaintSolution;
            break;
        }
        default: //do nothing
            break;
    }

    delay currMakespan =
            seedSolution.getASAPST()[dg.get_vertex({problemInstance.getJobsOutput().back(), 3}).id];
    DelayGraph::Edges bestSequence = builtSequence;

    while (builtSolution.getASAPST()[dg.get_vertex({problemInstance.getJobsOutput().back(), 3}).id]
           < currMakespan) {
            currMakespan =
                    builtSolution
                            .getASAPST()[dg.get_vertex({problemInstance.getJobsOutput().back(), 3})
                                                 .id]; // update before change
            bestSequence = builtSequence;
            std::tie(builtSequence, builtSolution) =
                    updateSequence(problemInstance, reEntrantMachine, builtSequence, dg, args);

            switch (args.algorithm) {
            case AlgorithmType::MINEH: {
                auto [builtMaintSolution, builtMaintDg] =
                            MaintenanceHeuristic::triggerMaintenance(dg, problemInstance, reEntrantMachine, builtSolution, args);
                builtSolution = builtMaintSolution;
                break;
        }
        default: //do nothing
            break;
        }
    }
    return bestSequence;
}

std::tuple<DelayGraph::Edges, PartialSolution>
MNEH::updateSequence(const FORPFSSPSD::Instance &problemInstance,
                     MachineId reEntrantMachine,
                     const DelayGraph::Edges &seedSequence,
                     delayGraph &dg,
                     const commandLineArgs &args) {

    //first option just goes into the built sequence directly
    DelayGraph::Edges builtSequence = {seedSequence[0]};
    std::vector<delay> builtASAPST;

    if(args.verbose >= LOGGER_LEVEL::DEBUG) {
        LOG(LOGGER_LEVEL::DEBUG, fmt::format(FMT_COMPILE("Updating sequence from seed sequence")));
        for (auto e: seedSequence){
            LOG(LOGGER_LEVEL::DEBUG,
                fmt::format(FMT_COMPILE("{} {} {}"),
                            dg.get_vertex(e.src).operation,
                            e,
                            dg.get_vertex(e.dst).operation));
        }
    }

    //initialise seed sequence performance
    auto ASAPST = algorithm::LongestPath::initializeASAPST(dg);
    PartialSolution seedSolution({{reEntrantMachine, seedSequence}}, ASAPST);
    auto finalSeedSequence = problemInstance.createFinalSequence(seedSolution);
    auto [resultseed, ASAPSTseed] = algorithm::LongestPath::computeASAPST(dg, finalSeedSequence);
    seedSolution.setASAPST(ASAPSTseed);    

    for (int j=1; j<seedSequence.size(); j++){
        auto curr_v = seedSequence[j].dst;

        DelayGraph::Edges testSequence;
        std::optional<DelayGraph::Edges> bestSequence;
        std::optional<std::vector<delay>> bestASAPST;
        delay minMakespan =
                seedSolution
                        .getASAPST()[dg.get_vertex({problemInstance.getJobsOutput().back(), 3}).id];

        for (int i=0; i<=builtSequence.size(); i++){
            testSequence = builtSequence;

            if (i < builtSequence.size()){
                // insert in position i of built sequence
                edge edgeInsert(builtSequence[i].src,
                                curr_v,
                                problemInstance.query(dg.get_vertex(builtSequence[i].src),
                                                      dg.get_vertex(curr_v)));
                edge edgeReplace(curr_v,
                                 builtSequence[i].dst,
                                 problemInstance.query(dg.get_vertex(curr_v),
                                                       dg.get_vertex(builtSequence[i].dst)));
                testSequence.insert(testSequence.begin()+i,edgeInsert); 
                testSequence[i+1] = edgeReplace;    
                // logging
                if(args.verbose >= LOGGER_LEVEL::DEBUG) {
                    LOG(LOGGER_LEVEL::DEBUG,
                        fmt::format(FMT_COMPILE("Inserting operation {} after {}"),
                                    dg.get_vertex(curr_v).operation,
                                    dg.get_vertex(builtSequence[i].src).operation));
            }
            }
            else{
                // insert at end of built sequence
            edge edgeInsert(builtSequence.back().dst,
                            curr_v,
                            problemInstance.query(dg.get_vertex(builtSequence.back().dst),
                                                  dg.get_vertex(curr_v)));
            testSequence.push_back(edgeInsert);
            // logging
            if (args.verbose >= LOGGER_LEVEL::DEBUG) {
                    LOG(LOGGER_LEVEL::DEBUG,
                        fmt::format(FMT_COMPILE("Inserting operation {} after {}"),
                                    dg.get_vertex(curr_v).operation,
                                    dg.get_vertex(builtSequence.back().dst).operation));
            }
            }
            // add the rest of the initial sequence
            // mend connection
            DelayGraph::Edges evaluateSequence;
            evaluateSequence = testSequence;
            if ((j+1) < seedSequence.size()){
            evaluateSequence.emplace_back(
                    testSequence.back().dst,
                    seedSequence[j + 1].dst,
                    problemInstance.query(dg.get_vertex(testSequence.back().dst),
                                          dg.get_vertex(seedSequence[j + 1].dst)));

            for (int k = j + 2; k < seedSequence.size(); k++) {
                    evaluateSequence.push_back(seedSequence[k]);
            }
            }        
  
            // compute if valid and better than others
            // only accept the responsible test sequence if evaluate has minimum makespan seen so far

            if (validateSequence(problemInstance, evaluateSequence, reEntrantMachine, dg)){
                auto ASAPST = algorithm::LongestPath::initializeASAPST(dg);
                PartialSolution solution({{reEntrantMachine, evaluateSequence}}, {ASAPST});
                auto final_sequence = problemInstance.createFinalSequence(solution);
                auto[result,ASAPSTr] = algorithm::LongestPath::computeASAPST(dg, final_sequence);
                solution.setASAPST(ASAPSTr);

                if (result.positiveCycle.empty()){
                    auto fullSolution = solution;
                    // switch (args.algorithm){
                    //     case AlgorithmType::MINEH: {
                    //         auto [builtMaintSolution, builtMaintDg] =
                    //         MaintenanceHeuristic::triggerMaintenance(dg, problemInstance,
                    //         reEntrantMachine, solution, args); fullSolution = builtMaintSolution;
                    //     }
                    //     default: {
                    //         //do nothing
                    //     }
                    // }
                    if (fullSolution.getASAPST()
                                [dg.get_vertex({problemInstance.getJobsOutput().back(), 3}).id]
                        < minMakespan) { //because maintenance can make getmakespan return wrong value since it just returns the end of ASAPST
                        // std::cout<<"found better " <<
                        // fullSolution.getASAPST()[dg.get_vertex({problemInstance.numberOfJobs-1,3}).id]
                        // << " than " << minMakespan << "\n";
                        bestSequence = testSequence;
                        bestASAPST = fullSolution.getASAPST();
                        minMakespan =
                                fullSolution.getASAPST()
                                        [dg.get_vertex({problemInstance.getJobsOutput().back(), 3})
                                                 .id];
                    }
                } 
            }
        }

        if (bestSequence.has_value()) {
            builtSequence = bestSequence.value();
            builtASAPST = bestASAPST.value();
        }
        else{
            builtSequence.emplace_back(
                    builtSequence.back().dst,
                    curr_v,
                    problemInstance.query(dg.get_vertex(builtSequence.back().dst),
                                          dg.get_vertex(curr_v)));
            builtASAPST = seedSolution.getASAPST();
        }

        if(args.verbose >= LOGGER_LEVEL::DEBUG) {
        LOG(LOGGER_LEVEL::DEBUG, fmt::format(FMT_COMPILE("Chose sub sequence")));
        for (auto e: builtSequence){
                LOG(LOGGER_LEVEL::DEBUG,
                    fmt::format(FMT_COMPILE("{} {} {}"),
                                dg.get_vertex(e.src).operation,
                                e,
                                dg.get_vertex(e.dst).operation));
            }
        }
    }
    PartialSolution builtSolution({{reEntrantMachine, builtSequence}}, builtASAPST);
    auto final_sequence = problemInstance.createFinalSequence(builtSolution);
    auto [result, ASAPSTr] = algorithm::LongestPath::computeASAPST(dg, final_sequence);
    builtSolution.setASAPST(ASAPSTr);
    return {builtSequence, builtSolution};
}

bool MNEH::validateSequence(const FORPFSSPSD::Instance &problemInstance,
                            const DelayGraph::Edges &sequence,
                            MachineId reEntrantMachine,
                            const delayGraph &dg) {

    bool status = true;
    // check how many operations are mapped
    const std::vector<OperationId> &ops = problemInstance.getMachineOperations(reEntrantMachine);

    std::optional<JobId> lastFirstPass;
    std::optional<JobId> lastSecondPass;
    std::vector<JobId> doneFirstPass;
    for (auto e: sequence){
        const auto &curr_v = dg.get_vertex(e.dst);
        auto curr_op = curr_v.operation;

        //confirm order of first passes
        if (curr_op.operationId == ops[0]) { //isFirstPass
            if (lastFirstPass.has_value() && curr_op.jobId <= lastFirstPass.value()){
                return false;
            }
            lastFirstPass = curr_op.jobId;
            doneFirstPass.push_back(curr_op.jobId);
        }

        //confirm order of second passes
        //confirm first second pass precedence
        if (curr_op.operationId == ops[1]){ //isSecondPass
            if(std::find(doneFirstPass.begin(), doneFirstPass.end(), curr_op.jobId) == doneFirstPass.end()){
                return false;
            }
            if(lastSecondPass.has_value() && curr_op.jobId <= lastSecondPass.value()) {
                return false;
            }
            lastSecondPass = curr_op.jobId;
        } 
    }
    return status;
}