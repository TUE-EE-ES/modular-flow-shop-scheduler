#include "pch/containers.hpp" // Precompiled headers always go first

#include "solvers/modular_args.hpp"

ModularArgs ModularArgs::fromArgs(const commandLineArgs &args) {
    ModularArgs result{.timer = FMS::StaticTimer(args.timeOut)};

    for (const auto &arg : args.modularAlgorithmOption) {
        const auto it = std::ranges::find(arg, '=');
        const std::string key(arg.begin(), it);

        std::string value;
        if (std::distance(it, arg.end()) > 1) {
            value = std::string(it + 1, arg.end());
        }

        if (key == kOptStoreBounds) {
            result.storeBounds = true;
        } else if (key == kOptStoreSequence) {
            result.storeSequence = true;
        } else if (key == kOptNoSelfBounds) {
            result.selfBounds = false;
        } else {
            throw std::invalid_argument(fmt::format(FMT_COMPILE("Unknown modular option {}"), key));
        }
    }
    return result;
}
