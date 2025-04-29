#include "fms/pch/containers.hpp"

#include "fms/cg/builder.hpp"

using namespace fms;
using namespace fms::cg;

namespace {
void addVerticesAndSources(ConstraintGraph &dg,
                           const problem::Instance &instance,
                           const std::vector<problem::JobId> &jobsOutput) {
    const auto &jobs = instance.jobs();
    const auto &machines = instance.getMachines();

    for (const auto m : machines) {
        // add a 'source' operation for each machine
        dg.addSource(m);
    }

    std::unordered_set<problem::MachineId> duplexFound;
    bool firstJob = true;
    // for each job, iterate and insert corresponding edges and vertices
    for (const auto j : jobsOutput) {
        const auto &jobOps = jobs.at(j);
        for (problem::Operation op : jobOps) {
            const auto vId = dg.addVertex(op);
            const problem::MachineId mId = instance.getMachine(op);
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
                auto &sourceV = dg.getSource(mId);
                auto &targetV = dg.getVertex(vId);
                dg.addEdge(sourceV, op, instance.query(sourceV, targetV));
            }
        }

        firstJob = false;
    }
}

void addIntraJobEdges(ConstraintGraph &dg,
                      const problem::Instance &instance,
                      const problem::OperationsVector &operations) {
    for (size_t i = 1; i < operations.size(); ++i) {
        const auto &op1 = operations[i - 1];
        const auto &op2 = operations[i];

        auto &v1 = dg.getVertex(op1);
        auto &v2 = dg.getVertex(op2);
        if (dg.hasEdge(v1, v2)) {
            // The edge was already defined in the explicit sequence-independent setup times
            continue;
        }

        const auto time = instance.query(op1, op2);
        const auto &e = dg.addEdge(v1, v2, time);
        LOG_D("Processing and setup time between ({}, {}) to be {}", op1, op2, e.weight);
    }
}

void addInterJobEdges(ConstraintGraph &dg,
                      const problem::Instance &instance,
                      const problem::OperationsVector &operations,
                      std::size_t jobIndex) {
    const auto &jobsOutput = instance.getJobsOutput();
    const auto &machines = instance.getMachines();
    const auto jobId = jobsOutput.at(jobIndex);

    // We'll need the ID of the first re-entrant machine

    auto firstReEntrant = instance.getFirstReEntrantId();

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
            const problem::JobId jobId2 = jobsOutput[jobIndex - j2];
            const problem::Operation op2{jobId2, op.operationId};

            // Check if the operation exists
            if (!instance.containsOp(op2)) {
                continue;
            }

            const problem::MachineId mId2 = instance.getMachine(op2);
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
            if (isFirstMachineOp && firstReEntrant
                && instance.getReEntrancies(jobId, *firstReEntrant)
                           != instance.getReEntrancies(jobId2, *firstReEntrant)) {
                continue;
            }

            // Both op and op2 exist so we can add the requirement and stop looking for jobs
            auto &v1 = dg.getVertex(op2);
            auto &v2 = dg.getVertex(op);

            // This is a sequence-dependent setup time already known so we can add it already
            dg.addEdge(v1, v2, instance.query(v1, v2));
            break;
        }
    }
}

void addSequenceIndependentSetupTimes(ConstraintGraph &dg, const problem::Instance &instance) {
    for (const auto &[opSrc, opDstMap] : instance.setupTimesIndep()) {
        const auto procTime = instance.processingTimes(opSrc);
        for (const auto &[opDst, setupTime] : opDstMap) {
            LOG_D("Processing and setup time between ({}, {}) to be {}", opSrc, opDst, setupTime);
            dg.addEdge(opSrc, opDst, procTime + setupTime);
        }
    }
}

void addSequenceIndependentDueDates(ConstraintGraph &dg, const problem::Instance &instance) {
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
            dg.addEdge(dg.getVertex(opSrc), dg.getVertex(op_dst), -dueDate);
        }
    }
}
} // namespace

cg::ConstraintGraph cg::Builder::customOrder(const problem::Instance &instance,
                                             const std::vector<problem::JobId> &jobOrder) {
    LOG("FORPFSSPSD is creating a delay graph");

    ConstraintGraph dg;

    const auto &jobs = instance.jobs();
    const auto &machines = instance.getMachines();

    addVerticesAndSources(dg, instance, jobOrder);
    addSequenceIndependentSetupTimes(dg, instance);

    for (std::size_t j = 0; j < jobs.size(); ++j) {
        const auto &operations = instance.jobs(jobOrder[j]);
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
            problem::Operation opSrc{jobOrder[i], 0};
            problem::Operation opDst{jobOrder[i + 1], 0};

            auto &vSrc = dg.getVertex(opSrc);
            auto &vDst = dg.getVertex(opDst);

            dg.addEdge(vSrc, vDst, instance.query(vSrc, vDst));
        }
    }
    return dg;
}

cg::ConstraintGraph cg::Builder::jobShop(const problem::Instance &problemInstance) {
    ConstraintGraph dg;

    // Job Shops are order-independent
    const auto &jobs = problemInstance.jobs();

    // Insert a source for each machine
    for (const auto &m : problemInstance.getMachines()) {
        dg.addSource(m);
    }

    // Insert all the operations of every job into the graph
    for (const auto &[j, jobOps] : jobs) {
        std::optional<problem::Operation> prevOp;
        VertexId prevVId = 0;

        for (problem::Operation op : jobOps) {
            const auto currV = dg.addVertex(op);
            if (prevOp) {
                const auto &e = dg.addEdge(prevVId, currV, problemInstance.query(*prevOp, op));
                LOG_D("Processing and setup time between ({}, {}) to be {}", op, *prevOp, e.weight);
            }

            prevVId = currV;
            prevOp = op;
        }
    }

    // add one terminal operation for all jobs
    dg.addTerminus();

    // And add an edge from the last operation of the job to the terminus
    cg::Vertex &terminus = dg.getTerminus();
    for (const auto &[j, jobOps] : jobs) {
        const auto lastOp = jobOps.back();
        auto e = dg.addEdge(lastOp, terminus, problemInstance.query(lastOp, terminus.operation));
        LOG_D("Terminal connection from {} of {} with weight {}", lastOp, terminus, e.weight);
    }

    // Insert all the sequence-independent relative deadlines
    for (const auto &[opSrc, opDstMap] : problemInstance.dueDatesIndep()) {
        for (const auto &[op_dst, dueDate] : opDstMap) {
            // add a negative value edge to the graph. This represents a due date, or a deadline
            auto e = dg.addEdge(dg.getVertex(opSrc), dg.getVertex(op_dst), -dueDate);
            LOG_D("Deadline between ({}, {}) to be {}", opSrc, op_dst, e.weight);
        }
    }

    // Insert all the absolute deadlines for the completion time of jobs
    for (const auto &[jobId, dueDate] : problemInstance.absoluteDueDates()) {
        const auto &jobOps = problemInstance.jobs(jobId);
        const auto &lastOp = jobOps.back(); // confirm last op correct
        const auto &lastMachine = problemInstance.getMachine(lastOp);
        // const auto &lastMachine = 0;

        // add a negative value edge to the graph. This represents a due date, or a deadline
        // add edge from last operation of job to first operation of that machine
        // correct for absolute deadline if all sources are constrained to start at 0

        for (const auto m : problemInstance.getMachines()) {
            auto e = dg.addEdge(dg.getVertex(lastOp), dg.getSource(m), -dueDate);
            LOG_D("Deadline between ({}, {}) to be {}",
                  lastOp,
                  dg.getSource(m).operation,
                  e.weight);
        }
    }

    if (problemInstance.shopType() != cli::ShopType::FIXEDORDERSHOP) {
        return dg;
    }

    // If it is a fixed order shop, then insert the edges that enforce the order
    // Currently adds edges according to the rule that all operations on the same level should
    // follow the order To DO: Mixed plexity scheduling will break this because simplex and duplex
    // jobs don't have this constraint
    const auto &jobsOutput = problemInstance.getJobsOutput();
    for (size_t i = 1; i < jobsOutput.size(); ++i) {
        const auto &jobOps = jobs.at(jobsOutput[i]);
        for (const auto op : jobOps) {
            if (op.operationId != 1 && op.operationId != 2) {
                problem::Operation opSrc{jobsOutput[i - 1], op.operationId};
                problem::Operation opDst{jobsOutput[i], op.operationId};

                auto &vSrc = dg.getVertex(opSrc);
                auto &vDst = dg.getVertex(opDst);

                dg.addEdge(vSrc, vDst, problemInstance.query(opSrc, opDst));
                LOG_D("Edge between ({}, {}) to be {}",
                      opSrc,
                      opDst,
                      problemInstance.query(opSrc, opDst));
            }
        }
    }

    return dg;
}
