#ifndef FMS_SOLVERS_ALGORITHMS_DATA_HPP
#define FMS_SOLVERS_ALGORITHMS_DATA_HPP

#include "fms/problem/indices.hpp"

#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>

namespace fms::solvers {
class AlgorithmsData {

public:
    inline void addData(const problem::ModuleId moduleId, nlohmann::json data) {
        m_data[moduleId].push_back(std::move(data));
    }

    nlohmann::json toJSON() const;

private:
    std::unordered_map<problem::ModuleId, std::vector<nlohmann::json>> m_data;
};
} // namespace fms::solvers

#endif // FMS_SOLVERS_ALGORITHMS_DATA_HPP
