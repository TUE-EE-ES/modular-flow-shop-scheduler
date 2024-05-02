#ifndef GEOMETRICSELECTIONOPERATOR_H
#define GEOMETRICSELECTIONOPERATOR_H

#include "partialsolution.h"

class GeometricSelectionOperator
{
    const unsigned int intermediate_solutions;
private:
    static inline double flatten(const PartialSolution &t) {
        return (double) t.getAverageProductivity()/1e6 * (double) t.getMakespanLastScheduledJob()/1e6;
    }

    static double valueAngle(const PartialSolution &t) {
        return atan(flatten(t));
    }
    static inline bool compareEntries(const PartialSolution &t1, const PartialSolution &t2);

public:
    explicit GeometricSelectionOperator(const unsigned int intermediate_solutions);
    std::vector<PartialSolution> reduce(std::vector<PartialSolution> values) const;
};

#endif // GEOMETRICSELECTIONOPERATOR_H
