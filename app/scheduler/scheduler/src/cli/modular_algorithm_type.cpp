#include "fms/pch/containers.hpp"
#include "fms/pch/utils.hpp"

#include "fms/cli/modular_algorithm_type.hpp"

using namespace fms;
using namespace fms::cli;

ModularAlgorithmType ModularAlgorithmType::parse(const std::string &name) {
    const std::string lowerCase = utils::strings::toLower(name);

    for (const auto algorithm : allAlgorithms) {
        if (ModularAlgorithmType(algorithm).shortName() == lowerCase) {
            return algorithm;
        }
    }

    throw std::runtime_error(fmt::format("Unknown modular algorithm type: {}", name));
}

std::string_view ModularAlgorithmType::description() const {
    switch (m_value) {
    case Value::BROADCAST:
        return "At every iteration, all modules exchange their constraints with all their "
               "neighbours.";
    case Value::COCKTAIL:
        return "Inspired by the cocktail-shaker sorting algorithm. It starts with one "
               "module exchanging constraints with its neighbour. Then the neighbour exchanges "
               "constraints with its neighbour, and so on. After the last module is reached, the "
               "process is reversed.";
    }

    return "";
}

std::string_view ModularAlgorithmType::shortName() const {
    switch (m_value) {
    case Value::BROADCAST:
        return "broadcast";
    case Value::COCKTAIL:
        return "cocktail";
    }

    return "";
}

std::string_view ModularAlgorithmType::fullName() const {
    switch (m_value) {
    case Value::BROADCAST:
        return "broadcast";
    case Value::COCKTAIL:
        return "cocktail";
    }

    return "";
}
