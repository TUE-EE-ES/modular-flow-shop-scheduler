#ifndef FMS_DD_COMPARATOR_HPP
#define FMS_DD_COMPARATOR_HPP

#include "fms/dd/dd_solution.hpp"
#include "fms/dd/vertex.hpp"

namespace fms::dd {

/**
 * @brief Class to compare vertices based on their ranking
 */
class CompareVerticesRanking {
public:
    float rankFactor;
    uint32_t totalOps;
    delay bestLowerBound;
    delay bestUpperBound;

    /**
     * @brief Construct a new CompareVerticesRanking object
     * @param solution The DDSolution object to get ranking factors from
     */
    explicit CompareVerticesRanking(const DDSolution &solution) :
        rankFactor(solution.rankFactor()),
        totalOps(solution.totalOps()),
        bestLowerBound(solution.bestLowerBound()),
        bestUpperBound(solution.bestUpperBound()) {}

    /**
     * @brief Operator to compare two vertices based on their ranking
     * @param a First vertex to compare
     * @param b Second vertex to compare
     * @return True if the rank of the first vertex is greater than the second
     */
    bool operator()(const SharedVertex &a, const SharedVertex &b) const {
        auto flTotalOps = static_cast<float>(totalOps);
        auto flBestUpperBound = static_cast<float>(bestUpperBound);
        auto aRank =
                rankFactor * (static_cast<float>(totalOps - a->vertexDepth()) / flTotalOps)
                + (1.0 - rankFactor) * (static_cast<float>(a->lowerBound()) / flBestUpperBound);
        auto bRank =
                rankFactor * (static_cast<float>(totalOps - b->vertexDepth()) / flTotalOps)
                + (1.0 - rankFactor) * (static_cast<float>(b->lowerBound()) / flBestUpperBound);
        // auto aRank =
        //         0.5 * (static_cast<float>(totalOps - a->vertexDepth()) / flTotalOps)
        //         + 0.3 * (static_cast<float>(a->lowerBound()) / flBestUpperBound)
        //         + 0.2 * ((float) rand() / (RAND_MAX));
        // auto bRank =
        //         0.5 * (static_cast<float>(totalOps - b->vertexDepth()) / flTotalOps)
        //         + 0.3 * (static_cast<float>(b->lowerBound()) / flBestUpperBound)
        //         + 0.2 * ((float) rand() / (RAND_MAX));
        return aRank > bRank;
    }
};

/**
 * @brief Class to compare vertices based on their lower bound and depth if tied
 */
class CompareVerticesLowerBound {
public:
    /**
     * @brief Operator to compare two vertices based on their lower bound
     * @param a First vertex to compare
     * @param b Second vertex to compare
     * @return True if the lower bound of the first vertex is greater than the second
     */
    bool operator()(const std::shared_ptr<Vertex> &a, const std::shared_ptr<Vertex> &b) const {
        if (a->lowerBound() == b->lowerBound()) {
            return a->vertexDepth() < b->vertexDepth();
        }
        return a->lowerBound() > b->lowerBound();
    }
};

/**
 * @brief Class to compare vertices based on their lower bound
 */
class CompareVerticesLowerBoundMin {
public:
    /**
     * @brief Operator to compare two vertices based on their lower bound
     * @param a First vertex to compare
     * @param b Second vertex to compare
     * @return True if the lower bound of the first vertex is less than the second
     */
    bool operator()(const std::shared_ptr<Vertex> &a, const std::shared_ptr<Vertex> &b) const {
        return a->lowerBound() < b->lowerBound();
    }
};

} // namespace fms::DD

#endif // FMS_DD_COMPARATOR_HPP