#include "pch/containers.hpp"

#include "FORPFSSPSD/bounds.hpp"

nlohmann::json FORPFSSPSD::toJSON(const FORPFSSPSD::IntervalSpec &bounds) {
    nlohmann::json result;

    for (const auto &[jobFrom, mapTo] : bounds) {
        nlohmann::json jobIntervals;
        for (const auto &[jobTo, interval] : mapTo) {
            nlohmann::json value{interval.min().value_or(-1), interval.max().value_or(-1)};
            jobIntervals[fmt::to_string(jobTo)] = std::move(value);
        }
        result[fmt::to_string(jobFrom)] = jobIntervals;
    }
    return result;
}

nlohmann::json FORPFSSPSD::toJSON(const FS::GlobalBounds &globalBounds) {
    nlohmann::json result;
    for (const auto &[moduleId, moduleIntervals] : globalBounds) {
        const auto moduleIdInt = FORPFSSPSD::ModuleId(moduleId);
        nlohmann::json value{{"in", toJSON(moduleIntervals.in)},
                             {"out", toJSON(moduleIntervals.out)}};
        result[fmt::to_string(moduleIdInt)] = std::move(value);
    }
    return result;
}

nlohmann::json FORPFSSPSD::toJSON(const std::vector<FS::GlobalBounds> &bounds) {
    nlohmann::json result;
    for (const auto &globalIntervals : bounds) {
        result.push_back(toJSON(globalIntervals));
    }
    return result;
}

FORPFSSPSD::IntervalSpec FORPFSSPSD::moduleBoundsFromJSON(const nlohmann::json &json) {
    FS::IntervalSpec result;

    for (const auto &[jobFrom, mapTo] : json.items()) {
        const FS::JobId jobFromId(std::stoi(jobFrom));
        for (const auto &[jobTo, intervalJson] : mapTo.items()) {
            const FS::JobId jobToId(std::stoi(jobTo));

            if (intervalJson.size() != 2) {
                throw std::invalid_argument(fmt::format(
                        FMT_COMPILE("Invalid interval size for job {} to {}"), jobFromId, jobToId));
            }

            std::optional<delay> valueMin = intervalJson.at(0).get<delay>();
            std::optional<delay> valueMax = intervalJson.at(1).get<delay>();

            if (valueMin < 0) {
                valueMin = std::nullopt;
            }

            if (valueMax < 0) {
                valueMax = std::nullopt;
            }

            FS::TimeInterval interval{valueMin, valueMax};
            result[jobFromId].insert({jobToId, interval});
        }
    }
    return result;
}

FS::GlobalBounds FORPFSSPSD::globalBoundsFromJSON(const nlohmann::json &json) {
    FS::GlobalBounds result;
    for (const auto &[moduleId, moduleIntervals] : json.items()) {
        auto moduleIdInt = FS::ModuleId(std::stoi(moduleId));
        result[moduleIdInt].in = moduleBoundsFromJSON(moduleIntervals.at("in"));
        result[moduleIdInt].out = moduleBoundsFromJSON(moduleIntervals.at("out"));
    }
    return result;
}

std::vector<FS::GlobalBounds> FORPFSSPSD::allGlobalBoundsFromJSON(const nlohmann::json &json) {
    if (!json.is_array()) {
        throw std::invalid_argument("Expected array of global intervals");
    }

    std::vector<FS::GlobalBounds> result;
    for (const auto &globalIntervals : json) {
        result.emplace_back(globalBoundsFromJSON(globalIntervals));
    }
    return result;
}
