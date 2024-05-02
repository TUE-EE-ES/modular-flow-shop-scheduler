#ifndef ENVIRONMENTALSELECTIONOPERATOR_H
#define ENVIRONMENTALSELECTIONOPERATOR_H

#include "partialsolution.h"

#include "pch/containers.hpp"

class EnvironmentalSelectionOperator
{
    unsigned int intermediate_solutions;
public:
    explicit EnvironmentalSelectionOperator(unsigned int intermediate_solutions);
    std::vector<PartialSolution> reduce(std::vector<PartialSolution> values) const;
};

#endif // ENVIRONMENTALSELECTIONOPERATOR_H
