#ifndef PARETOCULL_H
#define PARETOCULL_H

#include "pch/containers.hpp"

namespace algorithm {
    namespace pareto {
        template<typename T> std::vector<T> simple_cull(std::vector<T> solutions) {
            std::vector<T> pareto;

            std::vector<T> sorted = solutions; // O(tours.size()) or constant time if it is allowed to be a move action... but that would destroy the incoming tours!

            auto undecided = std::list<T>(sorted.begin(), sorted.end()); // O(tours.size())

            while(!undecided.empty()) { // Worst case: O(tours.size())
                const auto iter = undecided.begin();
                auto candidate = *iter;

                undecided.erase(iter); // O(1)
                auto solutioniter = undecided.begin(); // todo: is reuse of variables a good thing?
                while(solutioniter != undecided.end()) { // Worst case: O(tours.size())
                    const auto other = *solutioniter;
                    if(candidate<=other){ // remove other, as it is dominated and continue
                        //                    dominated_solutions.push_back(other); // O(1)
                        solutioniter = undecided.erase(solutioniter); // O(1); removal from linked list
                    } else {
                        if(other<=candidate){ // add tour to the dominated set, it is dominated by other. Continue with other.
                            //                        dominated_solutions.push_back(candidate); // O(1)
                            undecided.erase(solutioniter);
                            candidate = std::move(other); // continue with other, it dominates the candidate
                            solutioniter = undecided.begin();// start comparing from the start
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
    }
}

#endif // PARETOCULL_H
