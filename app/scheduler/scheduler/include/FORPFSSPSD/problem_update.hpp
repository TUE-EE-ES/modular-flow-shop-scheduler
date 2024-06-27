#include "delay.h"
#include "operation.h"

#include <tuple>
#include <vector>

namespace FORPFSSPSD {

enum class ConstraintType {
    D,  /// Due date
    SD, /// Sequence-dependent setup time
    SI  /// Sequence-independent setup time
};

using ProblemRemoval = std::tuple<operation, operation, ConstraintType>;
using ProblemInsertion = std::tuple<operation, operation, ConstraintType, delay>;

struct ProblemUpdate {
    std::vector<ProblemRemoval> removals;
    std::vector<ProblemInsertion> insertions;
};
} // namespace FORPFSSPSD