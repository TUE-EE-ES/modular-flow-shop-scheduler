#include "fms/pch/containers.hpp"
#include "fms/pch/utils.hpp"

#include "fms/cli/schedule_output_format.hpp"

using namespace fms;
using namespace fms::cli;

ScheduleOutputFormat ScheduleOutputFormat::parse(const std::string &name) {
    const std::string lowerCase = utils::strings::toLower(name);
    if (lowerCase == "json") {
        return ScheduleOutputFormat::JSON;
    }
    if (lowerCase == "cbor") {
        return ScheduleOutputFormat::CBOR;
    }
    throw std::runtime_error("Unknown schedule output format: " + name);
}

std::string_view ScheduleOutputFormat::shortName() const {
    switch (m_value) {
    case Value::JSON:
        return "json";
    case Value::CBOR:
        return "cbor";
    }

    return "";
}
