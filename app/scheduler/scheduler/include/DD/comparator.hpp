#ifndef DD_COMPARATOR_HPP
#define DD_COMPARATOR_HPP


#include "DD/dd_solution.hpp"
#include "DD/vertex.hpp"

namespace DD {

class CompareVerticesRanking {
public:
    float rankFactor;
    uint32_t totalOps;
    delay bestLowerBound;
    delay bestUpperBound;
    explicit CompareVerticesRanking(const DD::DDSolution &solution) :
        rankFactor(solution.rankFactor()),
        totalOps(solution.totalOps()),
        bestLowerBound(solution.bestLowerBound()),
        bestUpperBound(solution.bestUpperBound()) {}

    bool operator()(const std::shared_ptr<DD::Vertex> &a,
                    const std::shared_ptr<DD::Vertex> &b) const {
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

/* comparing vertices */
class CompareVerticesLowerBound {
public:
    bool operator()(const std::shared_ptr<DD::Vertex> &a,
                    const std::shared_ptr<DD::Vertex> &b) const {
        if (a->lowerBound() == b->lowerBound()) {
            return a->vertexDepth() < b->vertexDepth();
        }
        return a->lowerBound() > b->lowerBound();
    }
};

/* comparing vertices */
class CompareVerticesLowerBoundMin {
public:
    bool operator()(const std::shared_ptr<DD::Vertex> &a,
                    const std::shared_ptr<DD::Vertex> &b) const {
        return a->lowerBound() < b->lowerBound();
    }
};

} // namespace DD

#endif // DD_COMPARATOR_HPP