#include "fms/pch/containers.hpp"
#include "fms/pch/utils.hpp"

#include "fms/cli/multi_algorithm_behaviour.hpp"

using namespace fms;
using namespace fms::cli;

MultiAlgorithmBehaviour MultiAlgorithmBehaviour::parse(const std::string &shortName) {
    for (const auto behaviour : allBehaviours) {
        if (MultiAlgorithmBehaviour(behaviour).shortName() == shortName) {
            return behaviour;
        }
    }

    throw std::runtime_error(fmt::format("Unknown multi-algorithm behaviour: {}", shortName));
}

std::string_view MultiAlgorithmBehaviour::shortName() const {
    switch (m_value) {
    case Value::FIRST:
        return "first";
    case Value::LAST:
        return "last";
    case Value::INTERLEAVE:
        return "interleave";
    case Value::DIVIDE:
        return "divide";
    case Value::RANDOM:
        return "random";
    }

    return "";
}

std::string_view MultiAlgorithmBehaviour::description() const {
    switch (m_value) {
    case Value::FIRST:
        return "Use the first algorithm in the list.";
    case Value::LAST:
        return "Use the last algorithm in the list.";
    case Value::INTERLEAVE:
        return "Cycle through the algorithms in the list, assigning the first algorithm to the "
               "first module, the second algorithm to the second module, etc. If there are more "
               "modules than algorithms, start over from the first algorithm.";
    case Value::DIVIDE:
        return "Divide the modules into as many groups as there are algorithms. Assign each "
               "group of modules to an algorithm. If there are more algorithms than modules, then "
               "only the first algorithms until the number of modules is reached are used.";
    case Value::RANDOM:
        return "Randomly assign the algorithms to the modules.";
    }

    return "";
}
