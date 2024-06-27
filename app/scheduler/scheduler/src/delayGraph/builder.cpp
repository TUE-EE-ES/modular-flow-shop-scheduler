#include "pch/containers.hpp"

#include "delayGraph/builder.hpp"

using namespace FORPFSSPSD;
using namespace DelayGraph;
using namespace DelayGraph::Builder;

namespace {
void addVerticesAndSources(delayGraph &dg,
                           const Instance &instance,
                           const std::vector<JobId> &jobsOutput) {
    const auto &jobs = instance.jobs();
    const auto &machines = instance.getMachines();

    for (const auto m : machines) {
        // add a 'source' operation for each machine
        dg.add_source(m);
    }

    std::unordered_set<MachineId> duplexFound;
    bool firstJob = true;
    // for each job, iterate and insert corresponding edges and vertices
    for (const auto j : jobsOutput) {
        const auto &jobOps = jobs.at(j);
        for (operation op : jobOps) {
            const auto vId = dg.add_vertex(op);
            const MachineId mId = instance.getMachine(op);
            const auto machineReEntrancies = instance.getMachineMaxReEntrancies(mId);
            const auto reEntrancies = instance.getReEntrancies(op);

            // Insert an edge from the source to the first duplex job of each re-entrant machine
            bool addSource = false;
            if (reEntrancies == machineReEntrancies && duplexFound.find(mId) == duplexFound.end()) {
                duplexFound.insert(mId);
                addSource = true;
            }

            if (firstJob || addSource) {
                // Add an edge from the source to the first job, and the first duplex job of each
                // re-entrant machine
                auto &sourceV = dg.get_source(mId);
                auto &targetV = dg.get_vertex(vId);
                dg.add_edge(sourceV, op, instance.query(sourceV, targetV));
            }
        }

        firstJob = false;
    }
}

void addIntraJobEdges(delayGraph &dg,
                      const Instance &instance,
                      const OperationsVector &operations) {
    for (size_t i = 1; i < operations.size(); ++i) {
        const auto &op1 = operations[i - 1];
        const auto &op2 = operations[i];

        auto &v1 = dg.get_vertex(op1);
        auto &v2 = dg.get_vertex(op2);
        if (dg.has_edge(v1, v2)) {
            // The edge was already defined in the explicit sequence-independent setup times
            continue;
        }

        const auto time = instance.query(op1, op2);
        const auto &e = dg.add_edge(v1, v2, time);
        LOG_D("Processing and setup time between ({}, {}) to be {}", op1, op2, e.weight);
    }
}

void addInterJobEdges(delayGraph &dg,
                      const Instance &instance,
                      const OperationsVector &operations,
                      std::size_t jobIndex) {
    const auto &jobsOutput = instance.getJobsOutput();
    const auto &machines = instance.getMachines();
    const auto jobId = jobsOutput.at(jobIndex);

    for (const auto &op : operations) {
        const auto mId = instance.getMachine(op);

        const bool isFirstMachineOp = mId == machines.front();
        const bool isLastOpInMachine = op.operationId == instance.getMachineOperations(mId).back();

        // If the operation is from the input machine, we check the plexity to decide whether
        // to add an edge or not. To decide the plexity we use the earliest re-entrant machine
        // that can be found
        const auto reEntrancies = instance.getReEntrancies(op);

        // Find an operation in the previous job that is on the same machine and is the same
        // re-entrancy
        for (std::size_t j2 = 1; j2 <= jobIndex; ++j2) {
            const JobId jobId2 = jobsOutput[jobIndex - j2];
            const operation op2{jobId2, op.operationId};

            // Check if the operation exists
            if (!instance.containsOp(op2)) {
                continue;
            }

            const MachineId mId2 = instance.getMachine(op2);
            if (mId != mId2) {
                continue;
            }

            // Always connect the last operation of consecutive jobs in the same machine
            const bool isPreviousJob = j2 == 1;
            const bool mustConnect = isPreviousJob && isLastOpInMachine;

            // If the operation is done in a re-entrant machine, plexity must match
            if (instance.getReEntrancies(op2) != reEntrancies && !mustConnect) {
                continue;
            }

            // If it's the first operation, plexity of first re-entrant machine must match
            if (isFirstMachineOp
                && instance.getPlexity(jobId, ReEntrantId(0))
                           != instance.getPlexity(jobId2, ReEntrantId(0))) {
                continue;
            }

            // Both op and op2 exist so we can add the requirement and stop looking for jobs
            auto &v1 = dg.get_vertex(op2);
            auto &v2 = dg.get_vertex(op);

            // This is a sequence-dependent setup time already known so we can add it already
            dg.add_edge(v1, v2, instance.query(v1, v2));
            break;
        }
    }
}

void addSequenceIndependentSetupTimes(delayGraph &dg, const Instance &instance) {
    for (const auto &[opSrc, opDstMap] : instance.setupTimesIndep()) {
        const auto procTime = instance.processingTimes(opSrc);
        for (const auto &[opDst, setupTime] : opDstMap) {
            LOG(LOGGER_LEVEL::DEBUG,
                "Processing and setup time between ({}, {}) to be {}",
                opSrc,
                opDst,
                setupTime);
            dg.add_edge(opSrc, opDst, procTime + setupTime);
        }
    }
}

void addSequenceIndependentDueDates(delayGraph &dg, const Instance &instance) {
    for (const auto &[opSrc, opDstMap] : instance.dueDatesIndep()) {
        for (const auto &[op_dst, dueDate] : opDstMap) {
            // sanity check: a deadline cannot be set to a destination
            // operation that must by definition precede the source operation
            // N.B. a self-deadline can be ignored as well.
            if (opSrc.jobId <= op_dst.jobId && opSrc.operationId <= op_dst.operationId) {
                throw ParseException(fmt::format(
                        "Infeasible due date detected between {} and {}.", opSrc, op_dst));
            }

            // add a negative value edge to the graph. This represents a due date, or a deadline
            dg.add_edge(dg.get_vertex(opSrc), dg.get_vertex(op_dst), -dueDate);
        }
    }
}
} // namespace

DelayGraph::delayGraph
DelayGraph::Builder::customOrder(const FORPFSSPSD::Instance &instance,
                                 const std::vector<FORPFSSPSD::JobId> &jobOrder) {
    LOG("FORPFSSPSD is creating a delay graph");

    delayGraph dg;

    const auto &jobs = instance.jobs();
    const auto &machines = instance.getMachines();

    addVerticesAndSources(dg, instance, jobOrder);
    addSequenceIndependentSetupTimes(dg, instance);

    for (std::size_t j = 0; j < jobs.size(); ++j) {
        const auto &operations = instance.getJobOperations(jobOrder[j]);
        addIntraJobEdges(dg, instance, operations);

        if (j == 0) {
            // Because the edges are added backwards, we skip the first iteration
            continue;
        }
        addInterJobEdges(dg, instance, operations, j);
    }

    addSequenceIndependentDueDates(dg, instance);

    if (!instance.isOutOfOrder()) {
        for (std::size_t i = 0; i + 1 < jobs.size(); ++i) {
            // Add edges between the first operations so the input order is fixed
            operation opSrc{jobOrder[i], 0};
            operation opDst{jobOrder[i + 1], 0};

            auto &vSrc = dg.get_vertex(opSrc);
            auto &vDst = dg.get_vertex(opDst);

            dg.add_edge(vSrc, vDst, instance.query(vSrc, vDst));
        }
    }
    return dg;
}

DelayGraph::delayGraph DelayGraph::Builder::jobShop(const FORPFSSPSD::Instance &problemInstance) {
    delayGraph dg;

    // Job Shops are order-independent
    const auto &jobs = problemInstance.jobs();

    // Insert a source for each machine
    for (const auto &m : problemInstance.getMachines()) {
        dg.add_source(m);
    }

    // Insert all the operations of every job into the graph
    for (const auto &[j, jobOps] : jobs) {
        std::optional<operation> prevOp;
        VertexID prevVId = 0;

        for (operation op : jobOps) {
            const auto currV = dg.add_vertex(op);
            if (prevOp) {
                const auto &e = dg.add_edge(prevVId, currV, problemInstance.query(*prevOp, op));
                LOG(LOGGER_LEVEL::DEBUG,
                    "Processing and setup time between ({}, {}) to be {}",
                    op,
                    *prevOp,
                    e.weight);
            }
            
            prevVId = currV;
            prevOp = op;
        }
    }

    //add one terminal operation for all jobs
    dg.add_terminus();

    // And add an edge from the last operation of the job to the terminus
    DelayGraph::vertex &terminus = dg.get_terminus();
    for (const auto &[j, jobOps] : jobs) {
        const auto lastOp = jobOps.back();
        auto e = dg.add_edge(lastOp, terminus, problemInstance.query(lastOp, terminus.operation));
        LOG(LOGGER_LEVEL::DEBUG,
                    "Terminal connection from {} of {} with weight {}",
                    lastOp,
                    terminus,
                    e.weight);
    }

    // Insert all the sequence-independent relative deadlines
    for (const auto &[opSrc, opDstMap] : problemInstance.dueDatesIndep()) {
        for (const auto &[op_dst, dueDate] : opDstMap) {
            // add a negative value edge to the graph. This represents a due date, or a deadline
            auto e = dg.add_edge(dg.get_vertex(opSrc), dg.get_vertex(op_dst), -dueDate);
            LOG(LOGGER_LEVEL::DEBUG,
                    "Deadline between ({}, {}) to be {}",
                    opSrc,
                    op_dst,
                    e.weight);
        }
    }

    // Insert all the absolute deadlines for the completion time of jobs
    for (const auto &[jobId, dueDate] : problemInstance.absoluteDueDates()) {
        const auto &jobOps = problemInstance.getJobOperations(jobId);
        const auto &lastOp = jobOps.back(); //confirm last op correct
        const auto &lastMachine = problemInstance.getMachine(lastOp);
        // const auto &lastMachine = 0;

        // add a negative value edge to the graph. This represents a due date, or a deadline
        // add edge from last operation of job to first operation of that machine 
        // correct for absolute deadline if all sources are constrained to start at 0
  
        for (const auto m : problemInstance.getMachines() ) {
            auto e = dg.add_edge(dg.get_vertex(lastOp), dg.get_source(m),-dueDate);
                LOG(LOGGER_LEVEL::DEBUG,
                        "Deadline between ({}, {}) to be {}",
                        lastOp,
                        dg.get_source(m).operation,
                        e.weight);
        }
        
    }

    if (problemInstance.shopType() != ShopType::FIXEDORDERSHOP) {
        return dg;
    }

    // If it is a fixed order shop, then insert the edges that enforce the order
    // Currently adds edges according to the rule that all operations on the same level should follow the order
    // To DO: Mixed plexity scheduling will break this because simplex and duplex jobs don't have this constraint
    const auto &jobsOutput = problemInstance.getJobsOutput();
    for (size_t i = 1; i < jobsOutput.size(); ++i) {
        const auto &jobOps = jobs.at(jobsOutput[i]);
        for (const auto op : jobOps) {
            if(op.operationId!=1 && op.operationId!=2){
            operation opSrc{jobsOutput[i - 1], op.operationId};
            operation opDst{jobsOutput[i], op.operationId};

            auto &vSrc = dg.get_vertex(opSrc);
            auto &vDst = dg.get_vertex(opDst);

            dg.add_edge(vSrc, vDst, problemInstance.query(opSrc,opDst));
            LOG(LOGGER_LEVEL::DEBUG,
                        "Edge between ({}, {}) to be {}",
                        opSrc,
                        opDst,
                        problemInstance.query(opSrc,opDst));
            }
        }
    }

    return dg;
}
