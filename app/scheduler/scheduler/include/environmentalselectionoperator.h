#ifndef ENVIRONMENTALSELECTIONOPERATOR_H
#define ENVIRONMENTALSELECTIONOPERATOR_H

#include "partialsolution.h"

#include <vector>

class EnvironmentalSelectionOperator
{
    unsigned int intermediate_solutions;
public:
    explicit EnvironmentalSelectionOperator(unsigned int intermediate_solutions);
    [[nodiscard]] std::vector<PartialSolution> reduce(std::vector<PartialSolution> values) const;
};

#endif // ENVIRONMENTALSELECTIONOPERATOR_H
