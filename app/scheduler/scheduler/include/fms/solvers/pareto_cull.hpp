#ifndef FMS_SOLVERS_PARETO_CULL_HPP
#define FMS_SOLVERS_PARETO_CULL_HPP

#include <list>
#include <vector>

/// @brief Contains the simple_cull function for Pareto optimization.
namespace fms::solvers::pareto {

/**
 * @brief Performs a simple cull operation on a set of solutions.
 *
 * This function performs a simple cull operation on a set of solutions to find the Pareto optimal
 * set. The function uses a simple iterative approach to compare each solution with all others. If a
 * solution is found to be dominated by any other, it is removed from the set. The function
 * continues until no dominated solutions remain.
 *
 * @tparam T The type of the solutions. This type must support the <= operator for comparison.
 * @param solutions The set of solutions to cull.
 * @return A vector containing the Pareto optimal set of solutions.
 */
template <typename T> std::vector<T> simple_cull(std::vector<T> solutions) {
    std::vector<T> pareto;

    std::vector<T> sorted = solutions; // O(tours.size()) or constant time if it is allowed to be a
                                       // move action... but that would destroy the incoming tours!

    auto undecided = std::list<T>(sorted.begin(), sorted.end()); // O(tours.size())

    while (!undecided.empty()) { // Worst case: O(tours.size())
        const auto iter = undecided.begin();
        auto candidate = *iter;

        undecided.erase(iter);                    // O(1)
        auto solutioniter = undecided.begin();    // todo: is reuse of variables a good thing?
        while (solutioniter != undecided.end()) { // Worst case: O(tours.size())
            const auto other = *solutioniter;
            if (candidate <= other) { // remove other, as it is dominated and continue
                //                    dominated_solutions.push_back(other); // O(1)
                solutioniter = undecided.erase(solutioniter); // O(1); removal from linked list
            } else {
                if (other <= candidate) { // add tour to the dominated set, it is dominated by
                                          // other. Continue with other.
                    //                        dominated_solutions.push_back(candidate); // O(1)
                    undecided.erase(solutioniter);
                    candidate = std::move(other); // continue with other, it dominates the candidate
                    solutioniter = undecided.begin(); // start comparing from the start
                } else {
                    solutioniter++; // skip for now; not dominating, nor dominated
                }
            }
        }

        // not dominated by any other Pareto point, add it to the Pareto set
        pareto.push_back(candidate); // O(1)
    }

    return pareto;
}
} // namespace fms::solvers::pareto

#endif // FMS_SOLVERS_PARETO_CULL_HPP