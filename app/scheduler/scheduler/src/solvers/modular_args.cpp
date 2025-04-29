#include "fms/pch/containers.hpp" // Precompiled headers always go first

#include "fms/solvers/modular_args.hpp"

namespace fms::cli {

ModularArgs ModularArgs::fromArgs(const cli::CLIArgs &args) {
    return {
            .storeBounds = args.modularOptions.storeBounds,
            .storeSequence = args.modularOptions.storeSequence,
            .selfBounds = !args.modularOptions.noSelfBounds,
            .timer = utils::time::StaticTimer(args.modularOptions.timeOut),
            .maxIterations = args.modularOptions.maxIterations,
    };
}

} // namespace fms::cli
