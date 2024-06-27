/*
  * @author Eghonghon Eigbe
  * schedule repair in a computed schedule
  */
#include "pch/containers.hpp"

#include "repairschedule.h"

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "FORPFSSPSD/operation.h"
#include "delayGraph/delayGraph.h"
#include "delayGraph/vertex.h"
#include "longest_path.h"

using namespace algorithm;
using namespace std;
using namespace DelayGraph;

std::tuple<PartialSolution, delayGraph> RepairSchedule::repairScheduleOffline(const FORPFSSPSD::Instance &problemInstance, delayGraph& dg, PartialSolution solution, FORPFSSPSD::operation eligibleOperation, std::vector<delay> &ASAPST){
    const auto &maintPolicy = problemInstance.maintenancePolicy();
    FORPFSSPSD::MachineId machine = problemInstance.getMachine(eligibleOperation);
    const FORPFSSPSD::OperationId &firstReEntrantOp = problemInstance.getMachineOperations(machine).front();
    const FORPFSSPSD::OperationId &secondReEntrantOp = firstReEntrantOp + 1;

    LOG(std::string() + "This schedule became infeasible because of operation " + fmt::to_string(*solution.latest_edge(machine)) +
        " the preceeding edge is " + fmt::to_string(*(--solution.latest_edge(machine))) + "\n.");

    FS::JobId lastFirstPass = FS::JobId::max();
    FS::JobId lastCommittedSecondPass = FS::JobId::max();

    auto start = solution.getChosenEdges(machine).begin();

    //find last second pass before the offending operation - offending is from the last edge
    auto startRepair = findSecondToLastFirstPass(problemInstance,dg,solution,machine,std::distance(solution.getChosenEdges(machine).begin(),solution.latest_edge(machine)));
    lastFirstPass = startRepair.first;
    start = start + startRepair.second;

    //find the second to last first pass from the offending operation
    lastCommittedSecondPass = findLastCommittedSecondPass(problemInstance,dg,solution,machine,std::distance(solution.getChosenEdges(machine).begin(),start));

    //if start was not updated at all, i.e. we are stuck at the beginning of the schedule
    if (start == solution.getChosenEdges(machine).begin()){
        throw FmsSchedulerException(
                "No repair strategy can be applied. This is not possible in the Canon case");
    }

    LOG(fmt::format("Last 1st pass is {} with edge {} while last committed 2nd pass is {}.\n",
                    lastFirstPass,
                    *start,
                    lastCommittedSecondPass));

    //perform the actual repair
    //insert second passes earlier
    std::vector<FORPFSSPSD::operation>  insertions;
    for(FS::JobId i = (lastCommittedSecondPass == FS::JobId::max() ? FS::JobId(0) :lastCommittedSecondPass+1); 
        i<=eligibleOperation.jobId; 
        i++){
        if (i <= lastFirstPass && problemInstance.getPlexity(i) == FORPFSSPSD::Plexity::DUPLEX){//if second pass not in chosen edges add it
            insertions.push_back({i,secondReEntrantOp});
        }
    }

    std::tie(solution,dg) = insertRepair(problemInstance, dg, solution, eligibleOperation, 
    ASAPST, insertions, std::distance(solution.getChosenEdges(machine).begin(),start));

    //remove second passes later to prevent duplication of operations
    std::vector<FORPFSSPSD::operation> removals = insertions;
    for (const vertex &v : dg.get_maint_vertices()) {
        removals.push_back(v.operation);
    }

    std::tie(solution,dg) = removeRepair(problemInstance, dg, solution, eligibleOperation, ASAPST, 
        removals, std::distance(solution.getChosenEdges(machine).begin(),solution.latest_edge(machine))+1, solution.getChosenEdges(machine).size());

    //check if repair successful
    auto ASAPSTnew = algorithm::LongestPath::initializeASAPST(dg);
    auto resultRepair = recomputeSchedule(problemInstance,
                                          solution,
                                          maintPolicy,
                                          dg,
                                          solution.getChosenEdges(machine),
                                          ASAPSTnew);
    if(!resultRepair.positiveCycle.empty()) {
        LOG("Infeasible schedule generated after repair!\n");
        std::tie(solution,dg) = repairScheduleOffline(problemInstance, dg, solution, eligibleOperation, ASAPSTnew); //repair recursively
    }

    //adjust first feasible edge
    for(unsigned int i = 0; i < solution.getChosenEdges(machine).size(); i++){
        if (dg.get_vertex(solution.getChosenEdges(machine)[i].src).operation == eligibleOperation) {
            solution.setFirstFeasibleEdge(machine,i);
            break;
        }
    }
    solution.setASAPST(ASAPSTnew);
    solution.incrRepairCount();
    return std::make_tuple(solution,dg);
}

std::pair<FORPFSSPSD::JobId, DelayGraph::Edges::difference_type>
RepairSchedule::findSecondToLastFirstPass(const FORPFSSPSD::Instance &problemInstance,
                                          delayGraph &dg,
                                          const PartialSolution &solution,
                                          FORPFSSPSD::MachineId machine,
                                          DelayGraph::Edges::difference_type start) {
    FS::JobId lastFirstPass = FS::JobId::max();
    const FORPFSSPSD::OperationId &firstReEntrantOp = problemInstance.getMachineOperations(machine).front();
    const FORPFSSPSD::OperationId &secondReEntrantOp = firstReEntrantOp + 1;

    const auto &chosenEdges = solution.getChosenEdges(machine);

    auto startRepair = chosenEdges.begin();
    auto looper =
            chosenEdges.begin()
            + start; // loop back from the last insertion, this was the culprit to become infeasible

    bool updateFirst = true;
    bool updateSecond = false;
    unsigned int nrFirst = 0;
    for (; looper != chosenEdges.begin(); --looper) {
        if ((dg.get_vertex((*looper).dst).operation).operationId == firstReEntrantOp
            && !dg.is_maint(dg.get_vertex((*looper).dst).id)) {
            nrFirst++;
            lastFirstPass = (dg.get_vertex((*looper).dst).operation).jobId;
            if (nrFirst == 2){ //because we're always looking for the second earliest first pass
                updateSecond = true;
                updateFirst = false;
                startRepair = looper;
                ++startRepair;
                break;
            }
        }
    }
    return {lastFirstPass, std::distance(chosenEdges.begin(), startRepair)};
}

FORPFSSPSD::JobId
RepairSchedule::findLastCommittedSecondPass(const FORPFSSPSD::Instance &problemInstance,
                                            delayGraph &dg,
                                            const PartialSolution &solution,
                                            FORPFSSPSD::MachineId machine,
                                            int start) {
    FS::JobId lastCommittedSecondPass = FS::JobId::max();

    const FORPFSSPSD::OperationId &firstReEntrantOp = problemInstance.getMachineOperations(machine).front();
    const FORPFSSPSD::OperationId &secondReEntrantOp = firstReEntrantOp + 1;

    auto looper = solution.getChosenEdges(machine).begin() + start; 

    for(; looper != solution.getChosenEdges(machine).begin(); --looper){
        if ((dg.get_vertex((*looper).src).operation).operationId == secondReEntrantOp) {
            lastCommittedSecondPass = (dg.get_vertex((*looper).src).operation).jobId;
            break;
        }
    }
    return lastCommittedSecondPass;
}

std::tuple<PartialSolution, delayGraph>
RepairSchedule::insertRepair(const FORPFSSPSD::Instance &problemInstance,
                             delayGraph &dg,
                             PartialSolution solution,
                             FORPFSSPSD::operation eligibleOperation,
                             std::vector<delay> &ASAPST,
                             const std::vector<FORPFSSPSD::operation> &ops,
                             DelayGraph::Edges::difference_type start) {
    const auto &maintPolicy = problemInstance.maintenancePolicy();
    FORPFSSPSD::MachineId machine = problemInstance.getMachine(eligibleOperation);

    auto iter = solution.getChosenEdges(machine).begin() + start;

    for (auto op: ops){
        LOG(fmt::format("Adding second pass for operation {}\n", op));
        auto iterE = iter;
        auto afterE = iter;
        ++afterE;

        const auto &prevv = dg.get_vertex((*iterE).src);
        const auto &currv = dg.get_vertex(op); // add second pass
        const auto &nextv = dg.get_vertex((*iterE).dst);
        const auto &afterv = dg.get_vertex((*afterE).dst);

        delay prevcurrW, currnextW;
        //create the option
        if (dg.is_maint(prevv.id)) {
            prevcurrW = maintPolicy.getMaintDuration(prevv.operation.maintId);
        } else {
            prevcurrW = problemInstance.query(prevv,currv);
        }

        if (dg.is_maint(nextv.id)) {
            currnextW = problemInstance.query(currv,afterv);
        } else {
            currnextW = problemInstance.query(currv,nextv);
        }

        edge prev(prevv.id, currv.id, prevcurrW);
        edge next(currv.id, nextv.id, currnextW);

        option loop_opt(prev,
                        next,
                        prevv.id,
                        currv.id,
                        nextv.id,
                        std::distance(solution.getChosenEdges(machine).begin(), iter),
                        false);

        LOG(fmt::format("Adding {} between {} and {}.\n",
                        currv.operation,
                        prevv.operation,
                        nextv.operation));

        // add the edges from the options to the list
        solution = solution.add(machine,loop_opt, ASAPST);
        iter = solution.latest_edge(machine);
    }
    return std::make_tuple(solution,dg);
}

std::tuple<PartialSolution, delayGraph>
RepairSchedule::removeRepair(const FORPFSSPSD::Instance &problemInstance,
                             delayGraph &dg,
                             PartialSolution solution,
                             FORPFSSPSD::operation eligibleOperation,
                             std::vector<delay> &ASAPST,
                             std::vector<FORPFSSPSD::operation> ops,
                             std::size_t start,
                             std::size_t end,
                             bool afterLast) {

    const auto &maintPolicy = problemInstance.maintenancePolicy();
    FORPFSSPSD::MachineId machine = problemInstance.getMachine(eligibleOperation);
    
    auto i = start;
    auto j = start;
    while(j != end){

        if (std::find(ops.begin(),
                      ops.end(),
                      dg.get_vertex(solution.getChosenEdges(machine)[i].src).operation)
            != ops.end()) {

            LOG(std::string() + "Removing second pass for operation at " + std::to_string(i) + " " + fmt::to_string(solution.getChosenEdges(machine)[i]) + ".\n");

            delay prevnextW;
            const auto &prevv = dg.get_vertex(solution.getChosenEdges(machine)[i - 1].src);
            const auto &currv = dg.get_vertex(solution.getChosenEdges(machine)[i].src);
            const auto &nextv = dg.get_vertex(solution.getChosenEdges(machine)[i].dst);

            if (dg.is_maint(prevv.id)) {
                prevnextW = maintPolicy.getMaintDuration(prevv.operation.maintId);

                auto k = i + 1; // track after
                while (dg.is_maint((solution.getChosenEdges(machine)[k]).dst)) {
                    k++;
                }

                auto z = i - 2; // track before
                while (dg.is_maint((solution.getChosenEdges(machine)[z]).src)) {
                    z--;
                }
                auto afterE = solution.getChosenEdges(machine)[k];
                const auto &afterv = dg.get_vertex(afterE.dst);

                auto beforeE = solution.getChosenEdges(machine)[z];
                const auto &beforev = dg.get_vertex(beforeE.src);
                beforeE.weight = (dg.is_maint(beforeE.src))
                                         ? beforeE.weight
                                         : problemInstance.query(beforev, afterv);
            } else if (dg.is_maint(nextv.id)) {
                auto k = i+1;
                while (dg.is_maint((solution.getChosenEdges(machine)[k]).dst)){
                    k++;
                }
                auto afterE = solution.getChosenEdges(machine)[k];
                const auto &afterv = dg.get_vertex(afterE.dst);
                prevnextW = problemInstance.query(prevv,afterv);
            } else {
                prevnextW = problemInstance.query(prevv,nextv);
            }

            //create the option
            edge curr(prevv.id, nextv.id, prevnextW);
            edge next = curr; // removal should leave next edge unaffected, just initialised to
                              // current to build a proper option instance

            option rem_opt(curr, next, prevv.id, currv.id, nextv.id, i);
            solution = solution.remove(machine,rem_opt,ASAPST,afterLast);
            LOG(fmt::format("Removed {} before {}.\n", currv.operation, nextv.operation));

            if (dg.is_maint(currv.id)) {
                solution.setMaintCount(solution.getMaintCount()-1);
            }
        } else {
            i++;
        }
        j++;
    }
    return std::make_tuple(solution,dg);
}

LongestPathResult
RepairSchedule::recomputeSchedule(const FORPFSSPSD::Instance &problemInstance,
                                  PartialSolution &schedule,
                                  const FORPFSSPSD::MaintenancePolicy &maintPolicy,
                                  delayGraph &dg,
                                  const std::vector<edge> &inputEdges,
                                  vector<delay> &ASAPST) {
    const auto &v = dg.get_vertex(inputEdges[0].src);
    FORPFSSPSD::MachineId machine =
            dg.is_source(v) ? dg.get_source_machine(v) : problemInstance.getMachine(v.operation);
    std::vector<edge> edges;
    for(const auto& i : inputEdges){
        if(!dg.has_edge(i.src, i.dst)) {
            dg.add_edge(i);
            edges.push_back(i);
        }
        if(dg.is_maint(i.src)){
            delay dueWeight = maintPolicy.getMaintDuration((dg.get_vertex(i.src)).operation.maintId)
                              + maintPolicy.getMinimumIdle() - 1;
            edge dueEdge = dg.add_edge(i.dst, i.src, -dueWeight);
            edges.push_back(dueEdge);
        }
    }
    
    auto result = LongestPath::computeASAPST(dg, ASAPST);

    for(const auto& i : edges) {
        dg.remove_edge(i);
    } 
    schedule.setASAPST(ASAPST);
    return result;
}

