#ifndef DD_SOLUTION_HPP
#define DD_SOLUTION_HPP

#include "vertex.hpp"

#include <chrono>
#include <cstdint>
#include <limits>
#include <nlohmann/json.hpp>
#include <vector>

namespace fms::dd {

class DDSolution {

/**
 * @brief Class that keeps global information about a solution during the DD solve
 */
public:
    /**
     * @brief Construct a new DDSolution object
     * @param solveStart Start time of solve
     * @param rankFactor Ranking factor
     * @param totalOps Total operations
     * @param statesTerminated List of all solutions found in the search
     * @param bestUpperBound Best known upper bound
     * @param bestLowerBound Best known lower bound
     * @param solveData Solving data
     * @param optimal Optimality status
     */
    DDSolution(std::chrono::milliseconds solveStart,
               float rankFactor,
               std::uint32_t totalOps,
               std::vector<Vertex> statesTerminated = {},
               delay bestUpperBound = std::numeric_limits<delay>::max(),
               delay bestLowerBound = std::numeric_limits<delay>::min(),
               nlohmann::json solveData = nlohmann::json::object(),
               bool optimal = false) :
        m_solveStart(solveStart),
        m_rankFactor(rankFactor),
        m_totalOps(totalOps),
        m_statesTerminated(std::move(statesTerminated)),
        m_bestUpperBound(bestUpperBound),
        m_bestLowerBound(bestLowerBound),
        m_solveData(std::move(solveData)),
        m_optimal(optimal) {}

private:
    /// List of all solutions found in the search
    std::vector<Vertex> m_statesTerminated;

    /// Best known upper bound
    delay m_bestUpperBound;

    /// Best known lower bound
    delay m_bestLowerBound;

    /// Solving data
    nlohmann::json m_solveData;

    /// Start time of solve
    std::chrono::milliseconds m_solveStart;

    /// Optimality status
    bool m_optimal;

    float m_rankFactor;
    std::uint32_t m_totalOps;

public:
    [[nodiscard]] inline delay bestLowerBound() const noexcept { return m_bestLowerBound; }
    [[nodiscard]] inline delay bestUpperBound() const noexcept { return m_bestUpperBound; }

    /**
     * @brief Get the start time of solve
     * @return Start time of solve
     */
    [[nodiscard]] inline auto start() const noexcept { return m_solveStart; }

    /**
     * @brief Get the list of all solutions found in the search
     * @return List of all solutions found in the search
     */
    [[nodiscard]] inline const auto &getStatesTerminated() const noexcept {
        return m_statesTerminated;
    }
    [[nodiscard]] inline const auto &getSolveData() const noexcept { return m_solveData; }

    /**
     * @brief Get the optimality status
     * @return True if an optimal solution was found
     */
    [[nodiscard]] inline bool isOptimal() const noexcept { return m_optimal; }

    [[nodiscard]] inline float rankFactor() const noexcept { return m_rankFactor; }

    [[nodiscard]] inline std::uint32_t totalOps() const noexcept { return m_totalOps; }

    void setBestLowerBound (const delay newLowerBound){
        m_bestLowerBound = newLowerBound;
        m_solveData["lowerBound"] = m_bestLowerBound;
    }

    void setBestUpperBound (const delay newUpperBound){m_bestUpperBound = newUpperBound;}

    void addNewSolution(const Vertex &newSolution) {

        if (newSolution.lowerBound() < m_bestUpperBound) {
            m_statesTerminated.emplace_back(newSolution);
            m_bestUpperBound = newSolution.lowerBound();
            m_solveData["anytime-solutions"].push_back(
                    {std::chrono::duration<float>(utils::time::getCpuTime() - m_solveStart).count(),
                     newSolution.lowerBound()});
            m_solveData["anytime-bounds"].push_back(
                    {std::chrono::duration<float>(utils::time::getCpuTime() - m_solveStart).count(),
                     m_bestLowerBound});
        }
        if (newSolution.lowerBound() <= m_bestLowerBound) {
            m_optimal = true;
        }
    }
};

} // namespace fms::DD

#endif // DD_SOLUTION_HPP
