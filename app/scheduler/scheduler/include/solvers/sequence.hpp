#ifndef SOLVER_SEQUENCE_HPP
#define SOLVER_SEQUENCE_HPP

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "FORPFSSPSD/module.hpp"
#include "FORPFSSPSD/production_line.hpp"
#include "delayGraph/edge.h"
#include "partialsolution.h"
#include "solvers/production_line_solution.hpp"

#include <filesystem>
#include <nlohmann/json.hpp>

namespace algorithm::Sequence {

class SequenceStrings {
public:
    static constexpr auto kModules = "modules";
    static constexpr auto kMachineSequences = "machineSequences";
};

std::tuple<std::vector<PartialSolution>, nlohmann::json>
solve(FORPFSSPSD::Instance &f, const std::filesystem::path &sequenceFile);

std::tuple<std::vector<PartialSolution>, nlohmann::json>
solve(FORPFSSPSD::Instance &f, const commandLineArgs &args, std::uint64_t iteration = 0);

std::tuple<std::vector<PartialSolution>, nlohmann::json>
solve(FORPFSSPSD::Module &f, const commandLineArgs &args, std::uint64_t iteration = 0);

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

FMS::ProductionLineSequences loadProductionLineSequences(const nlohmann::json &json,
                                                         const FORPFSSPSD::ProductionLine &f);

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
 * Where $moduleId is the id of the module, and $machinesSequence is defined in @ref
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

nlohmann::json saveSingleSequence(const DelayGraph::Edges &sequence, const FORPFSSPSD::Instance &f);

nlohmann::json savePerMachineSequences(const PartialSolution::MachineEdges &sequences,
                                       const FORPFSSPSD::Instance &f);

nlohmann::json saveProductionLineSequences(const FMS::ModulesSolutions &solutions,
                                           const FORPFSSPSD::ProductionLine &f);

inline nlohmann::json saveProductionLineSequences(const FMS::ProductionLineSolution &solution,
                                                  const FORPFSSPSD::ProductionLine &f) {
    return saveProductionLineSequences(solution.getSolutions(), f);
}

nlohmann::json saveProductionLineSequences(const FMS::ProductionLineSequences &sequences,
                                           const FORPFSSPSD::ProductionLine &f);

} // namespace algorithm::Sequence

#endif