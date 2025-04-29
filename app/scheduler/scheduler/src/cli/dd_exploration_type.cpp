#include "fms/pch/containers.hpp"
#include "fms/pch/utils.hpp"

#include "fms/cli/dd_exploration_type.hpp"

#include "fms/utils/strings.hpp"

using namespace fms;
using namespace fms::cli;

DDExplorationType DDExplorationType::parse(const std::string &name) {
    const std::string lowerCase = utils::strings::toLower(name);

    if (lowerCase == "breadth") {
        return DDExplorationType::BREADTH;
    }
    if (lowerCase == "depth") {
        return DDExplorationType::DEPTH;
    }
    if (lowerCase == "best") {
        return DDExplorationType::BEST;
    }
    if (lowerCase == "static") {
        return DDExplorationType::STATIC;
    }
    if (lowerCase == "adaptive") {
        return DDExplorationType::ADAPTIVE;
    }
    throw std::runtime_error("Unknown shop type: " + name);
}

std::string_view DDExplorationType::shortName() const {
    switch (m_value) {
    case Value::BREADTH:
        return "breadth";
    case Value::DEPTH:
        return "depth";
    case Value::BEST:
        return "best";
    case Value::STATIC:
        return "static";
    case Value::ADAPTIVE:
        return "adaptive";
    }

    return "";
}
