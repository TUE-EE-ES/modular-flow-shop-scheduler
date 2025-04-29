#include "fms/pch/containers.hpp"
#include "fms/pch/utils.hpp"

#include "fms/cli/algorithm_type.hpp"

#include "fms/utils/strings.hpp"

namespace fms::cli {

std::string_view AlgorithmType::description() const {
    switch (m_value) {
    case Value::BHCS:
        return "Uses a bounded constraint graph to solve the scheduling problem.";
    case Value::MDBHCS:
        return "Like BHCS but uses a Pareto frontier to store the best solutions.";
    case Value::MIBHCS:
        return "Extension of BHCS to handle maintenance constraints.";
    case Value::MISIM:
        return "Simulates maintenance insertion at the end of BHCS.";
    case Value::ASAP:
        return "Algorithm that inserts the operation at the first place that is feasible.";
    case Value::ASAP_BACKTRACK:
        return "ASAP that backtracks when it cannot insert an operation.";
    case Value::MIASAP:
        return "Extension of ASAP that handles maintenance constraints.";
    case Value::MIASAPSIM:
        return "Simulates maintenance insertion at the end of ASAP.";
    case Value::MNEH:
        return "MNEH flowshop scheduling heuristic.";
    case Value::MNEH_ASAP:
        return "MNEH flowshop scheduling heuristic with ASAP for the initial sequence.";
    case Value::MNEH_ASAP_BACKTRACK:
        return "MNEH flowshop scheduling heuristic with ASAP for the initial sequence and "
               "backtracking.";
    case Value::MNEH_BHCS_COMBI:
        return "MNEH flowshop scheduling heuristic with BHCS for the initial sequence. This uses a "
               "combination of flexibility and productivity but with more weight on flexibility";
    case Value::MNEH_BHCS_FLEXIBLE:
        return "MNEH flowshop scheduling heuristic with BHCS for the initial sequence. This "
               "focuses on providing only the most flexible schedule.";
    case Value::MINEH:
        return "Maintenance aware MNEH flowshop scheduling heuristic.";
    case Value::MINEHSIM:
        return "Simulates maintenance insertion at the end of MNEH.";
    case Value::BRANCH_BOUND:
        return "Branch and bound solver.";
    case Value::GIVEN_SEQUENCE:
        return "Use a given sequence to generate a schedule.";
    case Value::ANYTIME:
        return "Anytime version of BHCS, the more time it has the better the solution. You can use "
               "the --time-out flag to select how much time the algorithm can use per operation.";
    case Value::ITERATED_GREEDY:
        return "Iterated greedy solver for n-re-entrancy. You can use the --time-out flag to "
               "select how much time the algorithm can use per operation.";
    case Value::DD:
        return "Uses a decision diagram to do an exhaustive search of the solution space. If it "
               "runs out of time, it gives the best solution found so far. You can use the "
               "--time-out flag to select how much time the algorithm can use per operation.";
    case Value::DDSeed:
        return "Variant of DD that uses BHCS to generate a seed solution or can accept a given "
               "seed solution with the --sequence-file flag.";
    case Value::SIMPLE:
        return "Simple scheduler that does not interleave operations.";
    }
    return "";
}

std::string_view AlgorithmType::shortName() const {
    switch (m_value) {
    case Value::BHCS:
        return "bhcs";
    case Value::MDBHCS:
        return "mdbhcs";
    case Value::MIBHCS:
        return "mibhcs";
    case Value::MISIM:
        return "misim";
    case Value::ASAP:
        return "asap";
    case Value::ASAP_BACKTRACK:
        return "asap-backtrack";
    case Value::MIASAP:
        return "miasap";
    case Value::MIASAPSIM:
        return "miasapsim";
    case Value::MNEH:
        return "mneh";
    case Value::MNEH_ASAP:
        return "mneh-asap";
    case Value::MNEH_ASAP_BACKTRACK:
        return "mneh-asap-backtrack";
    case Value::MNEH_BHCS_COMBI:
        return "mneh-bhcs-combi";
    case Value::MNEH_BHCS_FLEXIBLE:
        return "mneh-bhcs-flexible";
    case Value::MINEH:
        return "mineh";
    case Value::MINEHSIM:
        return "minehsim";
    case Value::BRANCH_BOUND:
        return "branch-bound";
    case Value::GIVEN_SEQUENCE:
        return "sequence";
    case Value::ANYTIME:
        return "anytime";
    case Value::ITERATED_GREEDY:
        return "iterated-greedy";
    case Value::DD:
        return "dd";
    case Value::DDSeed:
        return "ddseed";
    case Value::SIMPLE:
        return "simple";
    }

    return "";
}

std::string_view AlgorithmType::fullName() const {
    switch (m_value) {
    case Value::BHCS:
        return "forward heuristic";
    case Value::MDBHCS:
        return "pareto heuristic";
    case Value::MIBHCS:
        return "maintenance aware forward heuristic";
    case Value::MISIM:
        return "simulated maintenance forward heuristic";
    case Value::ASAP:
        return "asap forward heuristic (no ranking)";
    case Value::ASAP_BACKTRACK:
        return "asap forward heuristic with backtracking";
    case Value::MIASAP:
        return "maintenance aware ASAP forward heuristic";
    case Value::MIASAPSIM:
        return "simulated maintenance ASAP forward heuristic";
    case Value::MNEH:
        return "modified NEH (Nawaz-Enscore-Ham)";
    case Value::MNEH_ASAP:
        return "modified NEH with ASAP";
    case Value::MNEH_ASAP_BACKTRACK:
        return "modified NEH with ASAP and backtracking";
    case Value::MNEH_BHCS_COMBI:
        return "modified NEH with BHCS";
    case Value::MNEH_BHCS_FLEXIBLE:
        return "modified NEH with more flexible BHCS";
    case Value::MINEH:
        return "maintenance aware MHEH ";
    case Value::MINEHSIM:
        return "simulated maintenance MHEH";
    case Value::BRANCH_BOUND:
        return "branch & bound";
    case Value::GIVEN_SEQUENCE:
        return "given sequence";
    case Value::ANYTIME:
        return "anytime heuristic";
    case Value::ITERATED_GREEDY:
        return "iterated greedy solver";
    case Value::DD:
        return "decision diagram";
    case Value::DDSeed:
        return "decision diagram with seed";
    case Value::SIMPLE:
        return "simple non-interleaving scheduler";
    }

    return "";
}

AlgorithmType AlgorithmType::parse(std::string_view name) {
    const std::string lowerCase = utils::strings::toLower(name);

    for (const auto algorithm : allAlgorithms) {
        if (AlgorithmType(algorithm).shortName() == lowerCase) {
            return algorithm;
        }
    }

    throw std::runtime_error(fmt::format("Unknown algorithm type: {}", name));
}

} // namespace fms::cli
