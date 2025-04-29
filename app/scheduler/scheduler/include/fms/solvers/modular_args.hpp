#ifndef SOLVERS_MODULAR_ARGS_HPP
#define SOLVERS_MODULAR_ARGS_HPP

#include "fms/cli/command_line.hpp"
#include "fms/utils/time.hpp"

#include <string_view>

namespace fms::cli {

struct ModularArgs {
    static constexpr std::string_view kOptStoreBounds = "store-bounds";
    static constexpr std::string_view kOptStoreSequence = "store-sequence";
    static constexpr std::string_view kOptNoSelfBounds = "no-self-bounds";

    /// If true, the bounds are stored in the output JSON.
    bool storeBounds = false;

    /// If true, the sequence is stored in the output JSON.
    bool storeSequence = false;

    /// If true, the bounds that a module sends are also stored in the module itself.
    bool selfBounds = true;

    /// Timer handling maximum allowed timer for the modular algorithm
    utils::time::StaticTimer timer;

    /// Maximum number of iterations that the modular algorithm should perform
    std::uint64_t maxIterations = std::numeric_limits<std::uint64_t>::max();

    /**
     * @brief Parse command line arguments for the modular solver.
     * @details The arguments is a comma separated list of key=value pairs or just keys. The
     * following keys are supported:
     * - `store-bounds` - Store the bounds in the output JSON.
     * - `store-sequence` - Store the sequence in the output JSON.
     * - `no-self-bounds` - Do not store the bounds that a module sends in the module itself.
     * @param args Command line arguments.
     * @return ModularArgs
     */
    static ModularArgs fromArgs(const cli::CLIArgs &args);
};

} // namespace fms::cli

#endif
