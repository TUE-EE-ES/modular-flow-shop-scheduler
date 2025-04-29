#ifndef FMS_PROBLEM_PROBLEM_UPDATE_HPP
#define FMS_PROBLEM_PROBLEM_UPDATE_HPP

#include "operation.hpp"

#include "fms/delay.hpp"

#include <tuple>
#include <vector>

namespace fms::problem {

enum class ConstraintType {
    D,  /// Due date
    SD, /// Sequence-dependent setup time
    SI  /// Sequence-independent setup time
};

using ProblemRemoval = std::tuple<Operation, Operation, ConstraintType>;
using ProblemInsertion = std::tuple<Operation, Operation, ConstraintType, delay>;

struct ProblemUpdate {
    std::vector<ProblemRemoval> removals;
    std::vector<ProblemInsertion> insertions;
};
} // namespace fms::problem

#endif // FMS_PROBLEM_PROBLEM_UPDATE_HPP
