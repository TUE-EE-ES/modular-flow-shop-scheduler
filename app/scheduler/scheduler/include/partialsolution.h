#ifndef PARTIALSOLUTION
#define PARTIALSOLUTION

#include "FORPFSSPSD/indices.hpp"
#include "delayGraph/edge.h"
#include "schedulingoption.h"

#include "pch/containers.hpp"

#include <fmt/compile.h>
#include <utility>

class PartialSolution {
    public:
        using MachineEdges = std::unordered_map<FORPFSSPSD::MachineId, DelayGraph::Edges>;

    private:
        mutable MachineEdges m_chosenEdges;
        double ranking{-1};
        delay avgProd{-1};
        delay makespanLastScheduledJob{-1};
        delay earliestStartFutureOperation{-1};
        unsigned int nrOpsInLoop{0};
        mutable std::unordered_map<FORPFSSPSD::MachineId, std::size_t> m_lastInsertedEdge;
        mutable std::unordered_map<FORPFSSPSD::MachineId, std::size_t> m_firstFeasibleEdge;
        mutable std::unordered_map<FORPFSSPSD::MachineId, std::size_t> m_firstMaintEdge;
        std::vector<delay> ASAPST;
        int id, prev_id{-1};
        unsigned int maintCount{0};
        unsigned int repairCount{0};
        unsigned int reprintCount{0};

    public:
        PartialSolution(
                MachineEdges edges,
                std::vector<delay> ASAPST,
                std::unordered_map<FORPFSSPSD::MachineId, std::size_t> lastInsertedEdge = {},
                std::unordered_map<FORPFSSPSD::MachineId, std::size_t> firstFeasibleEdge = {},
                std::unordered_map<FORPFSSPSD::MachineId, std::size_t> firstMaintEdge = {}) :
            m_chosenEdges(std::move(edges)),
            m_lastInsertedEdge(std::move(lastInsertedEdge)),
            m_firstFeasibleEdge(std::move(firstFeasibleEdge)),
            m_firstMaintEdge(std::move(firstMaintEdge)),
            ASAPST(std::move(ASAPST)) {

            static int id = 0;
            this->id = id++;
        }

    [[nodiscard]] inline const DelayGraph::Edges &
    getChosenEdges(FORPFSSPSD::MachineId machineId) const {
        return m_chosenEdges.at(machineId);
    }

    [[nodiscard]] DelayGraph::Edges getAllChosenEdges() const;

    [[nodiscard]] inline const auto &getChosenEdgesPerMachine() const noexcept {
        return m_chosenEdges;
    }

    [[nodiscard]] inline auto &getChosenEdgesPerMachine() noexcept { return m_chosenEdges; }

    friend class fmt::formatter<PartialSolution>;

    /*
     * Returns true iff lhs dominates rhs
     */
    friend bool operator<= (const PartialSolution &lhs, const PartialSolution &rhs) {
        // Domination relationship for bi-objective values;
        // Apparently if they have _equal_ productivity and/or flexibility, then they can still be interesting points?
        return lhs.makespanLastScheduledJob <= rhs.makespanLastScheduledJob// minimize productivity penalty
//                && lhs.avgProd <= rhs.avgProd // minimize average productivity
                && lhs.earliestStartFutureOperation <= rhs.earliestStartFutureOperation // minimize earliest start time future operation
                && lhs.nrOpsInLoop >= rhs.nrOpsInLoop // maximize nr of jobs in the loop
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

    void setMaintCount(const unsigned int value){ maintCount = value; }
    void setRepairCount(const unsigned int value){ repairCount = value; }
    void setReprintCount(const unsigned int value){ reprintCount = value; }
    void incrMaintCount(){ maintCount++; }
    void incrRepairCount(){ repairCount++; }

    // TODO:Incorrect makespan when maintenance is added as they are added behind ordinary
    // operations and back() is then a maint start time
    [[nodiscard]] delay getMakespan() const { return !ASAPST.empty() ? ASAPST.back() : -1; };

    [[nodiscard]] PartialSolution add(FORPFSSPSD::MachineId machineId,
                                              const algorithm::option &c,
                                              const std::vector<delay> &ASAPST) const;

    [[nodiscard]] PartialSolution remove(FORPFSSPSD::MachineId machineId,
                                                 const algorithm::option &c,
                                                 const std::vector<delay> &ASAPST,
                                                 bool after = true) const;

    [[nodiscard]] DelayGraph::Edges::const_iterator
    first_possible_edge(FORPFSSPSD::MachineId machineId) const;

    [[nodiscard]] DelayGraph::Edges::const_iterator
    first_maint_edge(FORPFSSPSD::MachineId machineId) const;

    [[nodiscard]] DelayGraph::Edges::const_iterator
    latest_edge(FORPFSSPSD::MachineId machineId) const;

    [[nodiscard]] unsigned int getNrOpsInLoop() const { return nrOpsInLoop; }
    void setNrOpsInLoop(unsigned int nr) {
        nrOpsInLoop = nr;
    }
    void clearASAPST() {
        ASAPST.clear();
    }

    [[nodiscard]] inline const std::vector<delay> &getASAPST() const { return ASAPST; }

    void setASAPST(std::vector<delay> asapst) {
        ASAPST = std::move(asapst);
    }

    void setFirstFeasibleEdge(FORPFSSPSD::MachineId machineId, const unsigned int value){
        m_firstFeasibleEdge[machineId] = value;
    }

    void setFirstMaintEdge(FORPFSSPSD::MachineId machineId, const unsigned int value){
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
};

std::string chosenEdgesToString(const PartialSolution &solution, const DelayGraph::delayGraph &dg);
template <> struct fmt::formatter<PartialSolution> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const PartialSolution &sol, FormatContext &ctx) -> decltype(ctx.out()) {
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

template <> struct fmt::formatter<std::vector<PartialSolution>> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const std::vector<PartialSolution> &solutions, FormatContext &ctx)
            -> decltype(ctx.out()) {
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

#endif // PARTIALSOLUTION

