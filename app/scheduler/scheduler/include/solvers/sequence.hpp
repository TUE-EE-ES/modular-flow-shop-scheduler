#ifndef SOLVER_SEQUENCE_HPP
#define SOLVER_SEQUENCE_HPP

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "FORPFSSPSD/module.hpp"
#include "FORPFSSPSD/production_line.hpp"
#include "delayGraph/edge.h"
#include "partialsolution.h"
#include "solvers/production_line_solution.hpp"

#include <nlohmann/json.hpp>

/// Solver that uses a provided sequence using the `--sequence-file` command-line argument.
namespace algorithm::Sequence {

/**
 * @brief Constants used to save and load sequences from JSON.
 */
class SequenceStrings {
public:
    static constexpr auto kModules = "modules";
    static constexpr auto kMachineSequences = "machineSequences";

    /// @brief Top level object name of every sequence file
    /// @details For compatibility with the output JSON file which stores the sequence in a key
    /// called `sequence`.
    static constexpr auto kSequence = "sequence";
};

/**
 * @brief Solve the passed problem instance and return the sequences of operations per machine.
 * @param f Instance of a n-re-entrant flow-shop problem with setup times, due dates, and/or
 * maintenance times.
 * @param args Command line arguments.
 */
std::tuple<std::vector<PartialSolution>, nlohmann::json>
solve(FORPFSSPSD::Instance &f, const commandLineArgs &args, std::uint64_t iteration = 0);

/**
 * @brief Solve the passed problem instance and return the sequences of operations per machine.
 * @param f Instance of a modular n-re-entrant flow-shop problem with setup times, due dates, and/or
 * maintenance times.
 * @param args Command line arguments.
 */
std::tuple<std::vector<PartialSolution>, nlohmann::json>
solve(FORPFSSPSD::Module &f, const commandLineArgs &args, std::uint64_t iteration = 0);

nlohmann::json loadSequencesTop(const std::string &filename);

/**
 * @brief Loads a single sequence of operations (`$operationsSequence`) from a JSON stream.
 *
 * The format is something like:
 *
 * ```json
 * [
 *   ["$jobId", "$operationId"],
 *   ...
 * ]
 * ```
 *
 * Where `$jobId` is the id of the job and `$operationId` is the id of the operation.
 *
 * @param jsonSequence Json array to parse.
 * @param machineId Id of the machine whose sequence is being loaded. Some checks are performed to
 * ensure that the sequence is valid.
 * @param f Flow-shop instance.
 * @return DelayGraph::Edges
 */
DelayGraph::Edges loadSingleSequence(const nlohmann::json &jsonSequence,
                                     FORPFSSPSD::MachineId machineId,
                                     const FORPFSSPSD::Instance &f);

/**
 * @brief Load the sequence of operations of all machines (`$machinesSequence`) from a JSON.
 *
 * The format is something like:
 *
 * ```json
 * {
 *   "machineSequences": {
 *     "$machineId": $operationsSequence,
 *     ...
 *   }
 * }
 * ```
 * Or, in the case of different sequences per iteration:
 *
 * ```json
 * {
 *   "machineSequences": {
 *     "$machineId": {
 *       "$iteration": $operationsSequence,
 *       ...
 *     }
 *   }
 * }
 *
 * Where `$machineId` is the id of the machine, and $operationsSequence is a sequence of operations
 * as defined in @ref loadSingleSequence and `$iteration` is the id of the iteration. If the id
 * is not present, and the file format is the second one, then it will be chosen as `$iteration %
 * total_ids`. Otherwise, the @p iteration parameter is ignored.
 *
 * @param json JSON object where to read the sequence from
 * @param f Instance to check the sequence against
 * @param iteration Iteration to load the sequence from.
 * @return PartialSolution::MachineEdges Edges of the loaded sequence per machine
 */
PartialSolution::MachineEdges loadPerMachineSequences(const nlohmann::json &json,
                                                      const FORPFSSPSD::Instance &f,
                                                      std::uint64_t iteration);

/**
 * @brief Loads the sequence of operations of a single module from a JSON.
 *
 * The format is something like:
 *
 * ```
 * {
 *   "modules": {
 *     "$moduleId": $machinesSequence,
 *     ...
 *   }
 * }
 * ```
 *
 * Where $moduleId is the id of the module, and `$machinesSequence` is defined in @ref
 * loadPerMachineSequences.
 *
 * @param json JSON object where to read the sequence from
 * @param f Instance to check the sequence against
 * @param iteration Iteration to load the sequence from. Only used if the file format has
 * iterations.
 * @return PartialSolution::MachineEdges Edges of the loaded sequence per machine
 */
PartialSolution::MachineEdges loadSingleModuleSequence(const nlohmann::json &json,
                                                       const FORPFSSPSD::Module &f,
                                                       std::uint64_t iteration);

FMS::ProductionLineSequences loadProductionLineSequences(const nlohmann::json &json,
                                                         const FORPFSSPSD::ProductionLine &f);

/**
 * @brief Load the sequences of operations of all machines from a JSON file.
 * @details Assumes that the JSON file has a top level object with a key `sequence` containing the
 * sequences of operations.
 */
inline PartialSolution::MachineEdges loadPerMachineSequencesTop(const std::string &filename,
                                                                const FORPFSSPSD::Instance &f,
                                                                std::uint64_t iteration) {
    return loadPerMachineSequences(loadSequencesTop(filename), f, iteration);
}

/**
 * @brief Load the sequences of operations of all machines from a JSON file. Production line
 * variation.
 * @details Assumes that the JSON file has a top level object with a key `sequence` containing the
 * sequences of operations.
 */
inline PartialSolution::MachineEdges loadSingleModuleSequenceTop(const std::string &filename,
                                                                 const FORPFSSPSD::Module &f,
                                                                 std::uint64_t iteration) {
    return loadSingleModuleSequence(loadSequencesTop(filename), f, iteration);
}

/**
 * @brief Load the sequences of operations of a production line from a JSON file.
 * @details Assumes that the JSON file has a top level object with a key `sequence` containing the
 * sequences of operations.
 */
inline FMS::ProductionLineSequences
loadProductionLineSequencesTop(const std::string &filename, const FORPFSSPSD::ProductionLine &f) {
    return loadProductionLineSequences(loadSequencesTop(filename), f);
}

/**
 * @brief Saves a single sequence as a JSON object.
 * @details The format is the same as the one used in @ref loadSingleSequence.
 * @param sequence The sequence to be saved.
 * @param dg The delay graph associated with the sequence.
 * @return The saved sequence as a JSON object.
 */
nlohmann::json saveSingleSequence(const DelayGraph::Edges &sequence,
                                  const DelayGraph::delayGraph &dg);

/**
 * @brief Saves the sequence of operations of all machines as a JSON object.
 * @details The format is the same as the one used in @ref loadPerMachineSequences.
 * @param sequences The sequence to be saved.
 * @param dg The delay graph associated with the sequence.
 * @return The saved sequence as a JSON object.
 */
nlohmann::json savePerMachineSequences(const PartialSolution::MachineEdges &sequences,
                                       const DelayGraph::delayGraph &dg);

/**
 * @brief Produces a top level object with the sequences of operations of all machines.
 * @details This JSON object can directly be written to a file and the sequences can be loaded
 * back by the @ref solve function.
 * @return nlohmann::json A JSON object with a single key `sequence` containing the sequences of
 * operations.
 */
inline nlohmann::json savePerMachineSequencesTop(const PartialSolution::MachineEdges &sequences,
                                                 const DelayGraph::delayGraph &dg) {
    return {{SequenceStrings::kSequence, savePerMachineSequences(sequences, dg)}};
}

/**
 * @brief Saves the sequence of operations of a single module as a JSON object.
 * @details The format is the same as the one used in @ref loadSingleModuleSequence.
 * @param solution The solution of the production line with the sequences of all the modules.
 * @param p The production line problem.
 * @return The saved sequence as a JSON object.
 */
nlohmann::json saveProductionLineSequences(const FMS::ModulesSolutions &solution,
                                           const FORPFSSPSD::ProductionLine &p);

inline nlohmann::json saveProductionLineSequences(const FMS::ProductionLineSolution &solution,
                                                  const FORPFSSPSD::ProductionLine &f) {
    return saveProductionLineSequences(solution.getSolutions(), f);
}

nlohmann::json saveProductionLineSequences(const FMS::ProductionLineSequences &sequences,
                                           const FORPFSSPSD::ProductionLine &f);

/**
 * @brief Produces a top level object with the sequences of operations of all machines.
 * @details This JSON object can directly be written to a file and the sequences can be loaded
 * back by the @ref solve function.
 * @return nlohmann::jsons A JSON object with a single key `sequence` containing the sequences of
 * operations.
 */
template <typename T>
inline nlohmann::json saveProductionLineSequencesTop(T &&sequences,
                                                     const FORPFSSPSD::ProductionLine &f) {
    return {{SequenceStrings::kSequence,
             saveProductionLineSequences(std::forward<T>(sequences), f)}};
}

} // namespace algorithm::Sequence

#endif