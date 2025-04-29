#ifndef FMS_SOLVERS_SEQUENCE_HPP
#define FMS_SOLVERS_SEQUENCE_HPP

#include "partial_solution.hpp"
#include "production_line_solution.hpp"

#include "fms/cg/edge.hpp"
#include "fms/problem/flow_shop.hpp"
#include "fms/problem/module.hpp"
#include "fms/problem/production_line.hpp"

#include <nlohmann/json.hpp>

/**
 * @brief Solver that uses a provided sequence.
 * @details You can provide the sequence in the CLI using the flag `--sequence-file`. The solver
 * also provides functions to save and load sequences from JSON files.
 *
 * <details>
 * <summary>JSON Schema for the sequence JSON files:</summary>
 *
 * @include sequence.schema.json
 *
 * </details>
 */
namespace fms::solvers::sequence {

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
 * @param iteration Iteration number. If the input file has multiple sequences for this problem,
 * the iteration number is used to select the correct sequence.
 * @return std::tuple<std::vector<PartialSolution>, nlohmann::json> The first element is a vector
 * of feasible solutions for the problem. The second element is a JSON object with information
 * about the execution and the solution.
 */
std::tuple<std::vector<PartialSolution>, nlohmann::json>
solve(problem::Instance &f, const cli::CLIArgs &args, std::uint64_t iteration = 0);

/**
 * @brief Overload of @ref solve. Solve the passed problem instance and return the sequences of
 * operations per machine.
 * @details This overload is used for the flow-shop problem with modules.
 * @param f Instance of a modular n-re-entrant flow-shop problem with setup times, due dates, and/or
 * maintenance times.
 * @param args Command line arguments.
 * @param iteration Iteration number. If the input file has multiple sequences for this problem,
 * the iteration number is used to select the correct sequence.
 * @return std::tuple<std::vector<PartialSolution>, nlohmann::json> The first element is a vector
 * of feasible solutions for the problem. The second element is a JSON object with information
 * about the execution and the solution.
 */
std::tuple<std::vector<PartialSolution>, nlohmann::json>
solve(problem::Module &f, const cli::CLIArgs &args, std::uint64_t iteration = 0);

/**
 * @brief Loads the sequence of operations of a machine as edges from a JSON object.
 *
 * The object format is similar to:
 *
 * ```json
 * [
 *   ["$jobId", "$operationId"],
 *   ...
 * ]
 * ```
 *
 * Where `$jobId` is the id of the job and `$operationId` is the id of the operation. Formally,
 * it is defined as a JSON schema in `config/sequences.schema.json` as the definition
 * `machineSequence`. See @ref fms::solvers::sequence .
 *
 * @param jsonSequence Json array to parse.
 * @param machineId Id of the machine whose sequence is being loaded. Some checks are performed to
 * ensure that the sequence is valid.
 * @param f Flow-shop instance.
 * @return cg::Edges
 */
cg::Edges loadMachineEdges(const nlohmann::json &jsonSequence,
                           problem::MachineId machineId,
                           const problem::Instance &f);

/**
 * @brief Loads the sequence of operations of a machine from a JSON object.
 * @details The expected JSON object format is the same as the one described in @ref
 * loadMachineEdges . The parameters @p machineId and @p f are not required but used for sanity
 * checks.
 * @param jsonSequence JSON object containing the sequence of operations.
 * @param machineId Machine whose sequence we want to load.
 * @param f Flow-shop instance.
 * @return Sequence Loaded sequence
 */
Sequence loadMachineSequence(const nlohmann::json &jsonSequence,
                             problem::MachineId machineId,
                             const problem::Instance &f);

/**
 * @brief Load the sequence of operations of all machines as edges from a JSON object.
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
 * ```
 *
 * Where `$machineId` is the id of the machine, and `$operationsSequence` is a sequence of
 * operations as defined in @ref loadMachineSequence and `$iteration` is the id of the iteration. If
 * the id is not present, and the file format is the second one, then it will be chosen as
 * `$iteration % total_ids`. Otherwise, the @p iteration parameter is ignored. Formally, it is
 * defined as a JSON schema in `config/sequences.schema.json` as the definition `basicSequenceFile`.
 *
 * @param json JSON object where to read the sequence from
 * @param f Instance to check the sequence against
 * @param iteration Iteration to load the sequence from.
 * @return PartialSolution::MachineEdges Edges of the loaded sequence per machine
 */
MachineEdges loadAllMachinesEdges(const nlohmann::json &json,
                                  const problem::Instance &f,
                                  std::uint64_t iteration);

/**
 * @brief Load the sequence of operations of all machines from a JSON object.
 * @details The expected JSON object format is the same as the one described in @ref
 * loadAllMachinesEdges.
 *
 * If multiple iterations of sequences are present, the @p iteration parameter
 * is used to select the iteration to load. If @p iteration is higher than the number of iterations,
 * the modulo operation is used to select the iteration. The parameter @p f is not required but
 * used for sanity checks.
 * @param json JSON object containing the sequences of operations.
 * @param f Flow-shop instance.
 * @param iteration Iteration to load the sequence from if multiple iterations are present.
 * @return MachineSequences Loaded sequences for all machines.
 */
MachinesSequences loadAllMachinesSequences(const nlohmann::json &json,
                                           const problem::Instance &f,
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
 * Where $moduleId is the id of the module and must match the id of module @p f, and
 * `$machinesSequence` is defined in @ref loadAllMachinesSequences. Formally, it is defined as a
 * JSON schema in `config/sequences.schema.json` as the definition `modulesSequenceFile`.
 *
 * @param json JSON object where to read the sequence from
 * @param f Instance to check the sequence against
 * @param iteration Iteration to load the sequence from. Only used if the file format has
 * iterations.
 * @return PartialSolution::MachineEdges Edges of the loaded sequence per machine
 */
MachineEdges loadSingleModuleEdges(const nlohmann::json &json,
                                   const problem::Module &f,
                                   std::uint64_t iteration);

/**
 * @brief Load the sequence of operations of a single module from a JSON.
 * @details The expected JSON object format is the same as the one described in @ref
 * loadSingleModuleEdges.
 *
 * If multiple iterations of sequences are present, the @p iteration parameter
 * is used to select the iteration to load. If @p iteration is higher than the number of iterations,
 * the modulo operation is used to select the iteration. The parameter @p f is not required but
 * used for sanity checks.
 * @param json JSON object containing the sequences of operations.
 * @param f Module to check the sequence against.
 * @param iteration Iteration to load the sequence from if multiple iterations are present.
 * @return MachineSequences Loaded sequences for all machines.
 */
MachinesSequences loadSingleModuleSequences(const nlohmann::json &json,
                                            const problem::Module &f,
                                            std::uint64_t iteration);

/**
 * @brief Loads the sequences of operations of all modules from a JSON.
 *
 * The format is something like:
 *
 * ```json
 * {
 *   "modules": {
 *     "$moduleId": $machinesSequence,
 *   ...
 * }
 * ```
 *
 * Where `$moduleId` is the id of a module and `$machinesSequence` is the sequences of operations
 * per machine of the module. The format of `$machinesSequence` is defined in
 * @ref loadAllMachinesSequences. The pattern `"$moduleId": $machinesSequence` can appear multiple
 * times with different `$moduleId`. Formal definition is in `config/sequences.schema.json` as the
 * definition `modulesSequenceFile`.
 *
 * @param json Input JSON object.
 * @param f Production line problem.
 * @return ProductionLineSequences Loaded sequences of operations of the whole production line.
 */
ProductionLineEdges loadProductionLineEdges(const nlohmann::json &json,
                                            const problem::ProductionLine &f);

/**
 * @brief Loads the sequences object stored under @ref SequenceStrings::kSequence from a JSON file.
 * @throws FmsSchedulerException If the file does not contain the key @ref
 * SequenceStrings::kSequence or if the file doesn't exist.
 * @param filename Path to the JSON file.
 * @return nlohmann::json Loaded JSON object with the sequences of operations. Must be further
 * parsed to obtain the sequences.
 */
nlohmann::json loadSequencesTop(const std::string &filename);

/**
 * @brief Load the sequences of operations of all machines from a JSON file as edges.
 * @details Assumes that the JSON file has a top level object with a key `sequence` containing the
 * sequences of operations.
 */
inline MachineEdges loadAllMachinesEdgesTop(const std::string &filename,
                                            const problem::Instance &f,
                                            std::uint64_t iteration) {
    return loadAllMachinesEdges(loadSequencesTop(filename), f, iteration);
}

/**
 * @brief Load the sequences of operations of all machines from a JSON file.
 * @details Assumes that the JSON file has a top level object with a key `sequence` containing the
 * sequences of operations.
 */
inline MachinesSequences loadAllMachinesSequencesTop(const std::string &filename,
                                                     const problem::Instance &f,
                                                     std::uint64_t iteration) {
    return loadAllMachinesSequences(loadSequencesTop(filename), f, iteration);
}

/**
 * @brief Load the sequences of operations of all machines from a JSON file as edges. Production
 * line variation.
 * @details Assumes that the JSON file has a top level object with a key `sequence` containing the
 * sequences of operations.
 */
inline MachineEdges loadSingleModuleEdgesTop(const std::string &filename,
                                             const problem::Module &f,
                                             std::uint64_t iteration) {
    return loadSingleModuleEdges(loadSequencesTop(filename), f, iteration);
}

inline MachinesSequences loadSingleModuleSequencesTop(const std::string &filename,
                                                      const problem::Module &f,
                                                      std::uint64_t iteration) {
    return loadSingleModuleSequences(loadSequencesTop(filename), f, iteration);
}

/**
 * @brief Load the sequences of operations of a production line from a JSON file.
 * @details Assumes that the JSON file has a top level object with a key `sequence` containing the
 * sequences of operations.
 */
inline ProductionLineEdges loadProductionLineEdgesTop(const std::string &filename,
                                                      const problem::ProductionLine &f) {
    return loadProductionLineEdges(loadSequencesTop(filename), f);
}

/**
 * @brief Saves a single sequence as a JSON object.
 * @details The format is the same as the one used in @ref loadMachineSequence.
 * @param sequence The sequence to be saved.
 * @param dg The delay graph associated with the sequence.
 * @return The saved sequence as a JSON object.
 */
nlohmann::json saveMachineSequence(const cg::Edges &sequence, const cg::ConstraintGraph &dg);

/**
 * @brief Overload of @ref saveMachineSequence handling a sequence instead of edges.
 * @param sequence Sequence to convert to a JSON object.
 * @return nlohmann::json Sequence saved as a JSON object.
 */
nlohmann::json saveMachineSequence(const Sequence &sequence);

/**
 * @brief Overload of @ref saveMachineSequence with multiple iterations of sequences
 * @details This overload saves the sequence of a single machine but for multiple iterations. The
 * format is the same as the one used in @ref loadMachineSequence but with the multiple iterations
 * variant.
 * @param sequences Vector of sequences, where each element represents an iteration
 * @param dg Constraint graph associated with the sequences.
 * @return nlohmann::json The saved sequence as a JSON object.
 */
nlohmann::json saveMachineSequence(const std::vector<cg::Edges> &sequences,
                                   const cg::ConstraintGraph &dg);

/**
 * @brief Overload of @ref saveMachineSequence with multiple iterations of sequences and sequences
 * instead of edges.
 * @param sequences Vector of sequences, where each element represents an iteration
 * @return nlohmann::json The saved sequence as a JSON object.
 */
nlohmann::json saveMachineSequence(const std::vector<Sequence> &sequences);

/**
 * @brief Saves the sequence of operations of all machines as a JSON object.
 * @details The format is the same as the one used in @ref loadAllMachinesSequences.
 * @param sequences The sequence to be saved.
 * @param dg The delay graph associated with the sequence.
 * @return The saved sequence as a JSON object.
 */
nlohmann::json saveAllMachinesSequences(const MachineEdges &sequences,
                                        const cg::ConstraintGraph &dg);

/**
 * @brief Overload of @ref saveAllMachinesSequences with sequences instead of edges
 * @param sequences Sequences of all machines to save.
 * @return nlohmann::json JSON object containing the sequences of all machines.
 */
nlohmann::json saveAllMachinesSequences(const MachinesSequences &sequences);

/**
 * @brief Overload of @ref saveAllMachinesSequences with multiple iterations of sequences
 * @details This overload saves the sequence of all machines but for multiple iterations. The format
 * is the same as the one used in @ref loadAllMachinesSequences but with the multiple iterations
 * variant.
 * @param sequences Vector of sequences for all machines, where each element represents an
 * iteration.
 * @param dg Constraint graph associated with the sequences.
 * @return nlohmann::json The saved sequence as a JSON object.
 */
nlohmann::json saveAllMachinesSequences(const std::vector<MachineEdges> &sequences,
                                        const cg::ConstraintGraph &dg);

/**
 * @brief Overload of @ref saveAllMachinesSequences with multiple iterations of sequences and
 * sequences instead of edges.
 * @param sequences Vector of sequences for all machines, where each element represents an
 * iteration.
 * @return nlohmann::json JSON object containing the sequences of all machines.
 */
nlohmann::json saveAllMachinesSequences(const std::vector<MachinesSequences> &sequences);

/**
 * @brief Saves the sequence of operations of a single module as a JSON object.
 * @details The format is the same as the one used in @ref loadSingleModuleSequence.
 * @param solution The solution of the production line with the sequences of all the modules.
 * @param p The production line problem.
 * @return The saved sequence as a JSON object.
 */
nlohmann::json saveProductionLineSequences(const ModulesSolutions &solution,
                                           const problem::ProductionLine &p);

/**
 * @brief Overload of @ref saveProductionLineSequences with multiple iterations of sequences.
 * @details This overload saves the sequences of all modules but for multiple iterations.
 * @param solutions Vector of solutions, where each element represents an iteration.
 * @param p Production line problem associated with the solutions.
 * @return nlohmann::json Saved sequences of operations of all modules.
 */
nlohmann::json saveProductionLineSequences(const std::vector<ModulesSolutions> &solutions,
                                           const problem::ProductionLine &p);

/**
 * @brief Overload of @ref saveProductionLineSequences.
 * @details This overload handles the production line solution @p solution instead of the
 * collection of modules solutions.
 * @param solution Solution of the production line problem with the sequences of all the modules.
 * @param f The production line problem. Required to know the operations and machines.
 * @return nlohmann::json
 */
nlohmann::json saveProductionLineSequences(const ProductionLineSolution &solution,
                                           const problem::ProductionLine &f);

/**
 * @brief Overload of @ref saveProductionLineSequences.
 * @details This overload handles only the sequences @p sequences instead of a full solution.
 * @param sequences Sequences to store.
 * @param f Production line problem. Required to know the operations and machines.
 * @return nlohmann::json Object with the sequences of operations.
 */
nlohmann::json saveProductionLineSequences(const ProductionLineEdges &sequences,
                                           const problem::ProductionLine &f);

/**
 * @brief Produces a top level object with the sequences of operations of all machines.
 * @details The difference between this function and the @ref loadAllMachinesSequences is that this
 * function wraps the sequences in a top level object with the key @ref SequenceStrings::kSequence.
 * Thus, this object can be directly written directly to a file and loaded back by the @ref solve
 * function.
 * @return nlohmann::json A JSON object with a single key `sequence` containing the sequences of
 * operations.
 */
inline nlohmann::json saveAllMachinesSequencesTop(const MachineEdges &sequences,
                                                  const cg::ConstraintGraph &dg) {
    return {{SequenceStrings::kSequence, saveAllMachinesSequences(sequences, dg)}};
}

/**
 * @brief Overload of @ref saveAllMachinesSequencesTop with sequences instead of edges.
 * @param sequences Sequences of all machines to save.
 * @return nlohmann::json JSON object containing the sequences of all machines.
 */
inline nlohmann::json saveAllMachinesSequencesTop(const MachinesSequences &sequences) {
    return {{SequenceStrings::kSequence, saveAllMachinesSequences(sequences)}};
}

/**
 * @brief Produces a top level object with the sequences of operations of all machines.
 * @details This JSON object can directly be written to a file and the sequences can be loaded
 * back by the @ref solve function.
 * @return nlohmann::jsons A JSON object with a single key `sequence` containing the sequences of
 * operations.
 */
template <typename T>
inline nlohmann::json saveProductionLineSequencesTop(T &&sequences,
                                                     const problem::ProductionLine &f) {
    return {{SequenceStrings::kSequence,
             saveProductionLineSequences(std::forward<T>(sequences), f)}};
}

} // namespace fms::solvers::sequence

#endif