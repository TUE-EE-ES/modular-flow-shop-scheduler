#include "environmentalselectionoperator.h"
#include <algorithm>

EnvironmentalSelectionOperator::EnvironmentalSelectionOperator(const unsigned int intermediate_solutions)
    : intermediate_solutions(intermediate_solutions)
{
    if(intermediate_solutions <= 0) {
        throw std::runtime_error("It is invalid to request a reduction operator that reduces down to zero elements.");
    }
}

double distance(const PartialSolution &ps1, const PartialSolution &ps2, delay min_avg_prod, delay max_avg_prod, delay min_makespan, delay max_makespan, unsigned int min_nr_ops_in_buffer, unsigned int max_nr_ops_in_buffer) {
    return
            pow((ps1.getEarliestStartFutureOperation() - 2* min_avg_prod - ps2.getEarliestStartFutureOperation()) / (double) (max_avg_prod - min_avg_prod),2)
            + pow((ps1.getMakespanLastScheduledJob() - ps2.getMakespanLastScheduledJob() - 2* min_makespan) / (double) (max_makespan - min_makespan) , 2)
            + pow((ps1.getNrOpsInLoop() - ps2.getNrOpsInLoop() - 2* min_nr_ops_in_buffer) / (double) (max_nr_ops_in_buffer - min_nr_ops_in_buffer) , 2)
            ;
}

std::vector<PartialSolution> EnvironmentalSelectionOperator::reduce(std::vector<PartialSolution> values) const
{
    while(values.size() > intermediate_solutions) {
        // find maximum range of partial solution values:
        delay min_earliest_fut_sheet = std::numeric_limits<delay>::max(),
              max_earliest_fut_sheet = std::numeric_limits<delay>::min(),
              min_makespan = std::numeric_limits<delay>::max(),
              max_makespan = std::numeric_limits<delay>::min();

        unsigned int min_nr_ops_in_buffer = std::numeric_limits<unsigned int>::max(),
                max_nr_ops_in_buffer = std::numeric_limits<unsigned int>::min();

        for(PartialSolution &sol : values) {
            min_earliest_fut_sheet = std::min(min_earliest_fut_sheet, sol.getEarliestStartFutureOperation());
            max_earliest_fut_sheet = std::max(max_earliest_fut_sheet, sol.getEarliestStartFutureOperation());
            min_makespan = std::min(min_makespan, sol.getMakespanLastScheduledJob());
            max_makespan = std::max(max_makespan, sol.getMakespanLastScheduledJob());
            min_nr_ops_in_buffer = std::min(min_nr_ops_in_buffer, sol.getNrOpsInLoop());
            max_nr_ops_in_buffer = std::max(max_nr_ops_in_buffer, sol.getNrOpsInLoop());
        }

        // find the element with the smallest distance from any other
        // initialize the distance from solution i to solution j in a vector
        std::vector<std::vector<std::pair<double, unsigned int>>> d;
        d.reserve(values.size());
        for(unsigned int i = 0; i < values.size(); i++) {
            d.emplace_back();
            auto &dd = d[i];
            dd.reserve(values.size());

            for(unsigned int j = 0; j < values.size(); j++) {
                dd.emplace_back(distance(values[i], values[j], min_earliest_fut_sheet, max_earliest_fut_sheet, min_makespan, max_makespan, min_nr_ops_in_buffer, max_nr_ops_in_buffer), j);
            }
            // sort the vector, so that the closest neighbour is at the top of the list
            std::sort(dd.begin(), dd.end());
        }

        std::vector<unsigned int> eligible;
        for(unsigned int i = 0; i < values.size(); i++ ) {
            eligible.push_back(i);
        }

        for(unsigned int k = 1; k < values.size(); k++) { // k = 0 is the distance to the solution itself, which is 0 for every solution
            // find the minimum value for the k-th neighbour:
            double min = std::numeric_limits<double>::max();
            for(auto i : eligible) {
                min = std::min(min, d[i][k].first); // distance to k-th neighbour for solution i
            }
            std::vector<unsigned int> next_eligible;
            for(auto i : eligible) {
                if(d[i][k].first == min) {
                    next_eligible.push_back(i);
                }
            }
            eligible = next_eligible;
            if(eligible.size() == 1) {
                // found a single 'closest' item, we can stop
                break;
            }
        }
        values.erase(values.begin() + eligible.front()); // take the first one out (in case of absolute ties)
    }

    return values;
}

