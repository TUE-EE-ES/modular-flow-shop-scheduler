#include "pch/containers.hpp"
#include "pch/utils.hpp"

#include "geometricselectionoperator.h"

#include <algorithm>

GeometricSelectionOperator::GeometricSelectionOperator(const unsigned int intermediate_solutions)
    : intermediate_solutions(intermediate_solutions)
{
    if(intermediate_solutions <= 0) {
        throw std::runtime_error("It is invalid to request a reduction operator that reduces down to zero elements.");
    }
    LOG(LOGGER_LEVEL::DEBUG, std::string() + "reduction to " + std::to_string(intermediate_solutions));
}

std::vector<PartialSolution> GeometricSelectionOperator::reduce(std::vector<PartialSolution> values) const
{
    if(values.size() <= intermediate_solutions) {
        // do not apply reduction
        return values;
    }
    if(Logger::getVerbosity() >= LOGGER_LEVEL::DEBUG) {
        LOG(LOGGER_LEVEL::DEBUG, "reducing");

        for (auto & sol : values) {
            LOG(LOGGER_LEVEL::DEBUG, fmt::to_string(sol));
        }
    }

    std::vector<PartialSolution> result;

    std::sort(values.begin(), values.end(), compareEntries);

    auto start = valueAngle(values.front());
    auto end = valueAngle(values.back());

    auto range = end - start;

    auto stepsize = range / (intermediate_solutions - 1);

    auto iter = values.begin();
    // always use the first result in 2D space!
    result.push_back(*iter);
    iter++;

    for(unsigned int i = 1; i < intermediate_solutions; ++i) {
        auto limit = tan(start + stepsize * i); // there may be a loss of precision here
        auto &value = *iter;

        // find the first element that is not smaller
        for(;iter != values.end() && (flatten(value)) < limit; iter++) {
            value = *(iter);
        }

        result.push_back(value);

        if(iter == values.end()) {
            break;
        }
    }
    if(result.size() > intermediate_solutions) {
        throw std::runtime_error(std::string("Reduction operator did not reduce enough;") + std::to_string(values.size())
                                    + " was reduced to " + std::to_string(result.size()) + " which is still larger than " + std::to_string(intermediate_solutions));
    }
    return result;
}

bool GeometricSelectionOperator::compareEntries(const PartialSolution &t1, const PartialSolution &t2)
{
    return  valueAngle(t1) < valueAngle(t2);
}
