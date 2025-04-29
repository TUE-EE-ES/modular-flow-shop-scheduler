#include "fms/pch/containers.hpp"

#include "fms/problem/bounds.hpp"

using namespace fms;
using namespace fms::problem;

namespace {
template <typename T> nlohmann::json toJSON(const typename fms::math::Interval<T>::MaybeT &value) {
    if (value.has_value()) {
        return value.value();
    }
    return nullptr;
}

template <typename T> nlohmann::json toJSON(const fms::math::Interval<T> &interval) {
    return {toJSON<T>(interval.min()), toJSON<T>(interval.max())};
}
} // namespace

nlohmann::json problem::toJSON(const problem::IntervalSpec &bounds) {
    nlohmann::json result;

    for (const auto &[jobFrom, mapTo] : bounds) {
        nlohmann::json jobIntervals;
        for (const auto &[jobTo, interval] : mapTo) {
            jobIntervals[fmt::to_string(jobTo)] = ::toJSON(interval);
        }
        result[fmt::to_string(jobFrom)] = jobIntervals;
    }
    return result;
}

nlohmann::json problem::toJSON(const problem::GlobalBounds &globalBounds) {
    nlohmann::json result;
    for (const auto &[moduleId, moduleIntervals] : globalBounds) {
        const auto moduleIdInt = problem::ModuleId(moduleId);
        nlohmann::json value{{"in", toJSON(moduleIntervals.in)},
                             {"out", toJSON(moduleIntervals.out)}};
        result[fmt::to_string(moduleIdInt)] = std::move(value);
    }
    return result;
}

nlohmann::json problem::toJSON(const std::vector<problem::GlobalBounds> &bounds) {
    nlohmann::json result;
    for (const auto &globalIntervals : bounds) {
        result.push_back(toJSON(globalIntervals));
    }
    return result;
}

problem::IntervalSpec problem::moduleBoundsFromJSON(const nlohmann::json &json) {
    problem::IntervalSpec result;

    for (const auto &[jobFrom, mapTo] : json.items()) {
        const problem::JobId jobFromId(std::stoi(jobFrom));
        for (const auto &[jobTo, intervalJson] : mapTo.items()) {
            const problem::JobId jobToId(std::stoi(jobTo));

            if (intervalJson.size() != 2) {
                throw std::invalid_argument(fmt::format(
                        FMT_COMPILE("Invalid interval size for job {} to {}"), jobFromId, jobToId));
            }

            std::optional<delay> valueMin;
            std::optional<delay> valueMax;

            if (const auto &obj = intervalJson.at(0); !obj.is_null()) {
                valueMin = obj.get<delay>();
            }

            if (const auto &obj = intervalJson.at(1); !obj.is_null()) {
                valueMax = obj.get<delay>();
            }

            problem::TimeInterval interval{valueMin, valueMax};
            result[jobFromId].insert({jobToId, interval});
        }
    }
    return result;
}

problem::GlobalBounds problem::globalBoundsFromJSON(const nlohmann::json &json) {
    problem::GlobalBounds result;
    for (const auto &[moduleId, moduleIntervals] : json.items()) {
        auto moduleIdInt = problem::ModuleId(std::stoi(moduleId));
        result[moduleIdInt].in = moduleBoundsFromJSON(moduleIntervals.at("in"));
        result[moduleIdInt].out = moduleBoundsFromJSON(moduleIntervals.at("out"));
    }
    return result;
}

std::vector<problem::GlobalBounds> problem::allGlobalBoundsFromJSON(const nlohmann::json &json) {
    if (!json.is_array()) {
        throw std::invalid_argument("Expected array of global intervals");
    }

    std::vector<problem::GlobalBounds> result;
    for (const auto &globalIntervals : json) {
        result.emplace_back(globalBoundsFromJSON(globalIntervals));
    }
    return result;
}
