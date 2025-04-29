#ifndef FMS_SOLVERS_PARTIAL_SOLUTION_HPP
#define FMS_SOLVERS_PARTIAL_SOLUTION_HPP

#include "fms/cg/edge.hpp"
#include "fms/problem/aliases.hpp"
#include "fms/problem/indices.hpp"

#include "scheduling_option.hpp"

#include <fmt/compile.h>
#include <utility>

namespace fms::problem {
class Instance;
} // namespace fms::problem

namespace fms::solvers {
using Sequence = std::vector<problem::Operation>;
using MachinesSequences = std::unordered_map<problem::MachineId, Sequence>;
using MachineEdges = std::unordered_map<problem::MachineId, cg::Edges>;

/**
 * @class PartialSolution
 * @brief Class for handling partial solutions.
 */
class PartialSolution {
public:
    using SequenceDiff = MachinesSequences::mapped_type::difference_type;

    /**
     * @brief Constructor for the PartialSolution class.
     * @param edges The machine edges.
     * @param ASAPST The ASAPST vector.
     * @param lastInsertedEdge Map that contains, for each machine, the index of the last inserted
     * edge in @p edges .
     * @param firstFeasibleEdge Map that contains, for each machine, the index of the first feasible
     * edge in @p edges .
     * @param firstMaintEdge Map that contains, for each machine, the index of the first maintenance
     * edge in @p edges .
     * @return The PartialSolution object.
     * @details This constructor initializes the machine edges, the ASAPST vector, the last inserted
     * edge, the first feasible edge, and the first maintenance edge.
     */
    PartialSolution(MachinesSequences edges,
                    std::vector<delay> ASAPST,
                    std::unordered_map<problem::MachineId, std::size_t> lastInsertedEdge = {},
                    std::unordered_map<problem::MachineId, std::size_t> firstFeasibleEdge = {},
                    std::unordered_map<problem::MachineId, std::size_t> firstMaintEdge = {}) :
        m_chosenSequences(std::move(edges)),
        m_lastInsertedEdge(std::move(lastInsertedEdge)),
        m_firstFeasibleEdge(std::move(firstFeasibleEdge)),
        m_firstMaintEdge(std::move(firstMaintEdge)),
        ASAPST(std::move(ASAPST)) {

        static int id = 0;
        this->id = id++;
    }

    [[nodiscard]] inline const problem::OperationsVector &
    getMachineSequence(problem::MachineId machineId) const {
        return m_chosenSequences.at(machineId);
    }

    inline void setMachineSequence(problem::MachineId machineID, Sequence sequence) {
        m_chosenSequences[machineID] = std::move(sequence);
    }

    [[nodiscard]] cg::Edges getChosenEdges(problem::MachineId machineId,
                                           const problem::Instance &problem) const;
    [[nodiscard]] cg::Edges getAllChosenEdges(const problem::Instance &problem) const;

    [[nodiscard]] inline auto &getChosenSequencesPerMachine() noexcept { return m_chosenSequences; }

    [[nodiscard]] inline const auto &getChosenSequencesPerMachine() const noexcept {
        return m_chosenSequences;
    }

    Sequence getInferredInputSequence(const problem::Instance &problem) const;

    void addInferredInputSequence(const problem::Instance &problem);

    cg::Edges getAllAndInferredEdges(const problem::Instance &problem) const;

    friend class fmt::formatter<PartialSolution>;

    /**
     * @brief Comparison operator
     * @return true Iff. lhs dominates rhs
     * @return false If lhs does not dominate rhs
     */
    friend bool operator<=(const PartialSolution &lhs, const PartialSolution &rhs) {
        // Domination relationship for bi-objective values;
        // Apparently if they have _equal_ productivity and/or flexibility, then they can still be
        // interesting points?
        return lhs.makespanLastScheduledJob
                       <= rhs.makespanLastScheduledJob // minimize productivity penalty
               //                && lhs.avgProd <= rhs.avgProd // minimize average productivity
               && lhs.earliestStartFutureOperation
                          <= rhs.earliestStartFutureOperation // minimize earliest start time future
                                                              // operation
               && lhs.nrOpsInLoop >= rhs.nrOpsInLoop          // maximize nr of jobs in the loop
                ;
    }

    [[nodiscard]] inline long double getRanking() const { return ranking; }
    inline void setRanking(double value) { ranking = value; }

    inline void setAverageProductivity(delay value) { avgProd = value; }
    [[nodiscard]] inline delay getAverageProductivity() const { return avgProd; }

    inline void setMakespanLastScheduledJob(delay value) { makespanLastScheduledJob = value; }
    [[nodiscard]] inline delay getMakespanLastScheduledJob() const {
        return makespanLastScheduledJob;
    }

    unsigned int getMaintCount() const { return maintCount; }
    unsigned int getRepairCount() const { return repairCount; }
    unsigned int getReprintCount() const { return reprintCount; }

    void setMaintCount(const unsigned int value) { maintCount = value; }
    void setRepairCount(const unsigned int value) { repairCount = value; }
    void setReprintCount(const unsigned int value) { reprintCount = value; }
    void incrMaintCount() { maintCount++; }
    void incrRepairCount() { repairCount++; }

    [[nodiscard]] delay getMakespan() const { return !ASAPST.empty() ? ASAPST.back() : -1; };

    [[nodiscard]] PartialSolution add(problem::MachineId machineId,
                                      const SchedulingOption &c,
                                      const std::vector<delay> &ASAPST) const;

    [[nodiscard]] PartialSolution remove(problem::MachineId machineId,
                                         const SchedulingOption &c,
                                         const std::vector<delay> &ASAPST,
                                         bool after = true) const;

    [[nodiscard]] problem::OperationsVector::const_iterator
    firstPossibleOp(problem::MachineId machineId) const;

    [[nodiscard]] problem::OperationsVector::const_iterator
    firstMaintOp(problem::MachineId machineId) const;

    [[nodiscard]] problem::OperationsVector::const_iterator
    latestOp(problem::MachineId machineId) const;

    [[nodiscard]] unsigned int getNrOpsInLoop() const { return nrOpsInLoop; }
    void setNrOpsInLoop(unsigned int nr) { nrOpsInLoop = nr; }
    void clearASAPST() { ASAPST.clear(); }

    [[nodiscard]] inline const std::vector<delay> &getASAPST() const { return ASAPST; }

    void setASAPST(std::vector<delay> asapst) { ASAPST = std::move(asapst); }

    inline void setFirstFeasibleEdge(problem::MachineId machineId, const std::size_t value) {
        m_firstFeasibleEdge[machineId] = value;
    }

    inline void setFirstMaintEdge(problem::MachineId machineId, const std::size_t value) {
        m_firstMaintEdge[machineId] = value;
    }

    [[nodiscard]] int getId() const { return id; }

    [[nodiscard]] int getPrev_id() const { return prev_id; }

    void setEarliestStartFutureOperation(const delay &value) {
        earliestStartFutureOperation = value;
    }

    [[nodiscard]] delay getEarliestStartFutureOperation() const {
        return earliestStartFutureOperation;
    }

    [[nodiscard]] delay getRealMakespan(const problem::Instance &problem) const;

private:
    mutable MachinesSequences m_chosenSequences;
    double ranking{-1};
    delay avgProd{-1};
    delay makespanLastScheduledJob{-1};
    delay earliestStartFutureOperation{-1};
    unsigned int nrOpsInLoop{0};
    mutable std::unordered_map<problem::MachineId, std::size_t> m_lastInsertedEdge;
    mutable std::unordered_map<problem::MachineId, std::size_t> m_firstFeasibleEdge;
    mutable std::unordered_map<problem::MachineId, std::size_t> m_firstMaintEdge;
    std::vector<delay> ASAPST;
    int id, prev_id{-1};
    unsigned int maintCount{0};
    unsigned int repairCount{0};
    unsigned int reprintCount{0};
};

std::string chosenSequencesToString(const PartialSolution &solution);

} // namespace fms::solvers

template <> struct fmt::formatter<fms::solvers::PartialSolution> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const fms::solvers::PartialSolution &sol,
                FormatContext &ctx) const -> decltype(ctx.out()) {
        format_to(ctx.out(),
                  FMT_COMPILE("< makespan last scheduled job={}, makespan all jobs={}, avgProd={}, "
                              "earliest_fut_sheet={}, nrOpsInLoop={}, id={}, prev_id={}, last "
                              "operation ["),
                  sol.makespanLastScheduledJob,
                  sol.getMakespan(),
                  sol.avgProd,
                  sol.earliestStartFutureOperation,
                  sol.nrOpsInLoop,
                  sol.id,
                  sol.prev_id);
        for (const auto &[machineId, lastEdge] : sol.m_lastInsertedEdge) {
            format_to(ctx.out(), FMT_COMPILE(", ({}={})"), machineId, lastEdge);
        }
        return format_to(ctx.out(), FMT_COMPILE("]>"));
    }
};

template <>
struct fmt::formatter<std::vector<fms::solvers::PartialSolution>> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const std::vector<fms::solvers::PartialSolution> &solutions,
                FormatContext &ctx) const -> decltype(ctx.out()) {
        format_to(ctx.out(), "Makespan\tEarliestStart\tNrOps");
        for (const auto &sol : solutions) {
            format_to(ctx.out(),
                      FMT_COMPILE("{}\t{}\t{}"),
                      sol.getMakespanLastScheduledJob(),
                      sol.earliestStartFutureOperation,
                      sol.nrOpsInLoop);
        }
        return ctx.out();
    }
};

#endif // FMS_SOLVERS_PARTIAL_SOLUTION_HPP
