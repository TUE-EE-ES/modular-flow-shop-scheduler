#include "fms/pch/containers.hpp"

#include "fms/solvers/algorithms_data.hpp"

using namespace fms;
using namespace fms::solvers;

nlohmann::json AlgorithmsData::toJSON() const {
    nlohmann::json result;
    for (const auto &[moduleId, data] : m_data) {
        nlohmann::json moduleData;
        for (const auto &d : data) {
            moduleData.push_back(d);
        }
        result[fmt::to_string(moduleId.value)] = moduleData;
    }
    return result;
}
