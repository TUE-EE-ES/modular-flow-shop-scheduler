#include "maintenanceheuristic.h"

#include "FORPFSSPSD/indices.hpp"
#include "delayGraph/delayGraph.h"
#include "repairschedule.h"


using namespace algorithm;
using namespace std;
using namespace DelayGraph;

/*
In this version, maintenance operations are added with the assumption that maintenance and setup time
cannot overlap. To math the exact models, this should be changed.
*/

std::tuple<PartialSolution, delayGraph>
MaintenanceHeuristic::triggerMaintenance(delayGraph dg,
                                         const FORPFSSPSD::Instance &problemInstance,
                                         FORPFSSPSD::MachineId machine,
                                         const PartialSolution &solution,
                                         const commandLineArgs &args) {
    edge last_edge = solution.getChosenEdges(machine).back();
    FORPFSSPSD::operation eligibleOperation = dg.get_vertex(last_edge.src).operation;
    FORPFSSPSD::operation nextOperation = dg.get_vertex(last_edge.dst).operation;
    return triggerMaintenance(
            std::move(dg), problemInstance, solution, nextOperation, nextOperation, args);
}

std::tuple<PartialSolution, delayGraph>
MaintenanceHeuristic::triggerMaintenance(delayGraph dg,
                                         const FORPFSSPSD::Instance &problemInstance,
                                         const PartialSolution &solution,
                                         const option &eligibleOption,
                                         const commandLineArgs &args) {
    FORPFSSPSD::operation eligibleOperation = dg.get_vertex(eligibleOption.curV).operation;
    FORPFSSPSD::operation nextOperation = dg.get_vertex(eligibleOption.nextV).operation;
    return triggerMaintenance(
            std::move(dg), problemInstance, solution, eligibleOperation, nextOperation, args);
}

std::tuple<PartialSolution, delayGraph>
MaintenanceHeuristic::triggerMaintenance(delayGraph dg,
                                         const FORPFSSPSD::Instance &problemInstance,
                                         const PartialSolution &solution,
                                         FORPFSSPSD::operation eligibleOperation,
                                         FORPFSSPSD::operation nextOperation,
                                         const commandLineArgs &args) {
	FORPFSSPSD::MachineId reEntrantMachineId = problemInstance.getMachine(eligibleOperation);
	//iteratively evaluate the solution until no more maintenance needs to be added
	PartialSolution old_solution = solution;
    auto [updated_solution, updated_dg] = evaluateSchedule(
            problemInstance, dg, old_solution, eligibleOperation, nextOperation, args);
	while(updated_solution.getChosenEdges(reEntrantMachineId) != old_solution.getChosenEdges(reEntrantMachineId)){
		auto ASAPST = updated_solution.getASAPST();
		old_solution = updated_solution;
		std::tie(updated_solution,updated_dg) = evaluateSchedule(problemInstance, updated_dg,old_solution,eligibleOperation, nextOperation, args);
	}
    return {updated_solution, updated_dg};
}

std::tuple<PartialSolution, delayGraph>
MaintenanceHeuristic::evaluateSchedule(const FORPFSSPSD::Instance &problemInstance,
                                       delayGraph &dg,
                                       const PartialSolution &schedule,
                                       const FORPFSSPSD::operation &eligibleOperation,
                                       const FORPFSSPSD::operation &nextOperation,
                                       const commandLineArgs &args) {

    auto ASAPST = schedule.getASAPST();
    const auto &maintPolicy = problemInstance.maintenancePolicy();
    FORPFSSPSD::MachineId machine = problemInstance.getMachine(eligibleOperation);
    unsigned int lastCommittedSecondPass = std::numeric_limits<unsigned int>::max();

    auto totalSizes = problemInstance.getMaximumSheetSize();
    std::vector<delay> TLU(totalSizes + 1, 0);

    unsigned int i = std::distance(schedule.getChosenEdges(machine).begin(),
                                   schedule.first_maint_edge(machine));
    for (; i < schedule.getChosenEdges(machine).size()
           && dg.get_vertex(schedule.getChosenEdges(machine)[i].src).operation != eligibleOperation;
         i++) {
                // compute idle time trying to consider sheet properties
                auto fetch = fetchIdle(problemInstance, machine, dg, schedule, ASAPST, TLU, i);

                if (dg.get_vertex(schedule.getChosenEdges(machine)[i].src).operation.operationId
                    == 2) {
                    lastCommittedSecondPass =
                            dg.get_vertex(schedule.getChosenEdges(machine)[i].src).operation.jobId;
                }

                // add operation to the delay graph and add necessary edges both to delay graph and
                // schedule
                unsigned int actionID = checkInterval(fetch, maintPolicy, args);
                if (actionID != delayGraph::MAINT_ID) {
                    LOG("Maintenance triggered before op {}",dg.get_vertex(schedule.getChosenEdges(machine)[i].dst).operation);
                    // add the edges from the options to the list
                    auto q = insertMaintenance(
                            problemInstance, machine, dg, schedule, ASAPST, i, actionID);
                    auto new_solution = q.first;
                    dg = q.second;
                    new_solution.incrMaintCount();

                    ASAPST.push_back(std::numeric_limits<delay>::min());
                    auto sources =
                            (lastCommittedSecondPass == std::numeric_limits<unsigned int>::max())
                                    ? VerticesCRef{dg.get_vertex(FORPFSSPSD::operation{0, 0})}
                                    : dg.cget_vertices(lastCommittedSecondPass);
                    auto window = dg.get_vertices(lastCommittedSecondPass + 1, nextOperation.jobId);
                    auto m = dg.get_maint_vertices();
                    window.insert(window.end(), m.begin(), m.end());

                    auto ASAPSTold = ASAPST; // stash old ASAPST
                    auto result = recomputeSchedule(problemInstance,
                                                    new_solution,
                                                    maintPolicy,
                                                    dg,
                                                    new_solution.getChosenEdges(machine),
                                                    ASAPST,
                                                    sources,
                                                    window);

                    if (!result.positiveCycle.empty()) {
                        LOG("Schedule Repair Triggered.");
                        std::tie(new_solution, dg) = RepairSchedule::repairScheduleOffline(
                                problemInstance, dg, new_solution, eligibleOperation, ASAPST);
                    }
                    return std::make_tuple(new_solution, dg);
                }
    }
    return std::make_tuple(schedule, dg);
}

std::pair<PartialSolution, delayGraph>
MaintenanceHeuristic::insertMaintenance(const FORPFSSPSD::Instance &problemInstance,
                                        FORPFSSPSD::MachineId machine,
                                        delayGraph &dg,
                                        const PartialSolution &schedule,
                                        std::vector<delay> &ASAPST,
                                        int i,
                                        unsigned int actionID) {

    const auto &maintPolicy = problemInstance.maintenancePolicy();
    delay time = maintPolicy.getMaintDuration(actionID);
    const FORPFSSPSD::OperationId &firstReEntrantOp =
            problemInstance.getMachineOperations(machine).front();

    VertexID maint = dg.add_maint(firstReEntrantOp, actionID); // create maintenance vertex

    delay oldweight = schedule.getChosenEdges(machine)[i].weight; // fetch weight of old connection
    delay oldProcTime = problemInstance.getProcessingTime(
            (dg.get_vertex(schedule.getChosenEdges(machine)[i].src)).operation);

    // add the new maintenance action to the computed schedule
    edge prev(schedule.getChosenEdges(machine)[i].src, maint, oldweight);
    edge next(maint, schedule.getChosenEdges(machine)[i].dst, time);

    // scheduling option creation arguments prevE(prevE), nextE(nextE), prevV(prevV),
    // curV(curV), nextV(nextV), insertion_point(insertion_point)
    const auto &prevv = dg.get_vertex(schedule.getChosenEdges(machine)[i].src);
    const auto &currv = dg.get_vertex(maint);
    const auto &nextv = dg.get_vertex(schedule.getChosenEdges(machine)[i].dst);

    option maint_opt(prev,
                     next,
                     prevv.id,
                     currv.id,
                     nextv.id,
                     std::distance(schedule.getChosenEdges(machine).begin(),
                                   schedule.getChosenEdges(machine).begin() + i),
                     true);
    ASAPST; // create a local copy that we can modify without issues

    // add the edges from the options to the list
            auto new_solution = schedule.add(machine, maint_opt, ASAPST);
            return {std::move(new_solution), dg};
}

std::pair<long, long> MaintenanceHeuristic::fetchIdle(const FORPFSSPSD::Instance &problemInstance,
                                                      FORPFSSPSD::MachineId machine,
                                                      delayGraph &dg,
                                                      const PartialSolution &schedule,
                                                      std::vector<delay> &ASAPST,
                                                      std::vector<delay> &TLU,
                                                      int i) {
    const auto &maintPolicy = problemInstance.maintenancePolicy();
    auto totalSizes = problemInstance.getMaximumSheetSize();
    const auto &edges = schedule.getChosenEdges(machine);

    if (dg.is_source(edges[i].src)) {
		for(int j=totalSizes; j>=0; j--){ 
			TLU[j] = 0;
		}
	}
	else if (dg.is_maint(edges[i].dst)){ //maintenance resets everybody
		for(int j=totalSizes; j>=0; j--){ 
			TLU[j] = 0;
		}
        } else if (dg.is_maint(edges[i].src)) {
            for (int j = totalSizes; j >= 0; j--) {
                TLU[j] = ASAPST[edges[i].dst] - ASAPST[edges[i].src]
                         - maintPolicy.getMaintDuration(
                                 (dg.get_vertex(edges[i].src)).operation.maintId);
            }
        }
        // reset TLU of this sheet size and all smaller sheet sizes to 0 and increment TLU of all
        // bigger sizes
        else {
            for (int j = problemInstance.getSheetSize((dg.get_vertex(edges[i].src)).operation);
                 j >= 0;
                 j--) {
                TLU[j] = ASAPST[edges[i].dst] - ASAPST[edges[i].src]
                         - problemInstance.getProcessingTime(
                                 (dg.get_vertex(edges[i].src)).operation);
            }
            for (int j = problemInstance.getSheetSize((dg.get_vertex(edges[i].src)).operation) + 1;
                 j <= totalSizes;
                 j++) {
                TLU[j] = TLU[j] + ASAPST[edges[i].dst] - ASAPST[edges[i].src];
            }
        }

        // check if maintenance should be triggered before the destination sheet use size of
        // destination sheet to choose
        long idle, maxidle = 0;
	if (dg.is_maint(edges[i].dst)){
		idle = TLU[0]; 
	} else {
		auto uniqueSheetSizes = problemInstance.getUniqueSheetSizes();
		for(int j = totalSizes; j >= 0; j--){
			if ((uniqueSheetSizes.find(j) != uniqueSheetSizes.end()) && TLU[j] > maxidle){
				maxidle = TLU[j];
			}
		}
                idle = TLU[problemInstance.getSheetSize((dg.get_vertex(edges[i].dst)).operation)];
        }

        return {idle, maxidle};
}

unsigned int MaintenanceHeuristic::checkInterval(std::pair<long,long> idle, const FORPFSSPSD::MaintenancePolicy& maintPolicy,const commandLineArgs &args){
    // Gets an interval and the policy returns id of triggered action
    auto interval = idle.first;
	auto maxinterval = idle.second;
    for (unsigned int i = 0; i < maintPolicy.getNumberOfTypes(); i++) {
        const auto &[minV, maxV] = maintPolicy.getThresholds(i);

        switch (args.algorithm){
	        case AlgorithmType::MIBHCS:
			case AlgorithmType::MINEH:
			case AlgorithmType::MIASAP:
	        	if ( ((interval >= minV) && (interval < maxV)) 
		          || ((maxinterval >= 0.9*maxV) && (maxinterval < maxV)) ) {
		            return i;
		        }
	        	break;
	        case AlgorithmType::MISIM:
			case AlgorithmType::MINEHSIM:
			case AlgorithmType::MIASAPSIM:
		        if ( ((interval >= minV) && (interval < maxV)) ) {
	            	return i;
	        	}
	            break;
	        default:
	        	throw FmsSchedulerException(std::string() + "Algorithm not recognised for maintenance insertion.");
	    }
    }
    return delayGraph::MAINT_ID;
}

LongestPathResult
MaintenanceHeuristic::recomputeSchedule(const FORPFSSPSD::Instance &problemInstance,
                                        PartialSolution &schedule,
                                        const FORPFSSPSD::MaintenancePolicy &maintPolicy,
                                        delayGraph &dg,
                                        const std::vector<edge> &inputEdges,
                                        vector<delay> &ASAPST,
                                        const VerticesCRef& sources,
                                        const VerticesCRef& window) {
    const auto &v = dg.get_vertex(inputEdges[0].src);
    FORPFSSPSD::MachineId machine =
            dg.is_source(v) ? dg.get_source_machine(v) : problemInstance.getMachine(v.operation);
    std::vector<edge> edges;
    for (const auto &i : inputEdges) {
        if (!dg.has_edge(i.src, i.dst)) {
            dg.add_edge(i);
            edges.push_back(i);
        }
        if (dg.is_maint(i.src)) {
            delay dueWeight = maintPolicy.getMaintDuration((dg.get_vertex(i.src)).operation.maintId)
                              + maintPolicy.getMinimumIdle() - 1;
            edge dueEdge = dg.add_edge(i.dst, i.src, -dueWeight);
            edges.push_back(dueEdge);
        }
    }
    auto result = LongestPath::computeASAPST(dg, ASAPST, sources, window);

    for (const auto &i : edges) {
        dg.remove_edge(i);
    }
    schedule.setASAPST(ASAPST);
    return result;
}
