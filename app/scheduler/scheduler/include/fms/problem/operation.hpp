#ifndef FMS_PROBLEM_OPERATION_HPP
#define FMS_PROBLEM_OPERATION_HPP

#include "indices.hpp"

#include "fms/algorithms/hash.hpp"

#include <cstdint>
#include <fmt/compile.h>

namespace fms::problem {
// inner types for temporary storage and mappings

/// @brief Order in which jobs should output the last machine
using JobOutOrder = std::size_t;

/// @brief Index of a maintenance class
using MaintType = std::uint32_t;

/// @brief Represents an operation
struct Operation {
    constexpr const static auto kJobIdDefault = JobId::max();

    JobId jobId{kJobIdDefault}; ///< @brief The ID of the job
    OperationId operationId{}; ///< @brief The ID of the operation
    std::optional<MaintType> maintId; ///< @brief The ID of the maintenance type

    /// @brief Compares this op
    /// eration with another for ordering
    /// @param rhs The other operation to compare with
    /// @return True if this operation is less than the other, false otherwise
    bool operator<(const Operation &rhs) const {
        return std::tie(jobId, operationId) < std::tie(rhs.jobId, rhs.operationId);
    }

    /// @brief Checks if this operation is equal to another
    /// @param op The other operation to compare with
    /// @return True if the operations are equal, false otherwise
    bool operator==(const Operation &op) const {
        return op.jobId == this->jobId && op.operationId == this->operationId;
    }

    /// @brief Checks if this operation is not equal to another
    /// @param op The other operation to compare with
    /// @return True if the operations are not equal, false otherwise
    bool operator!=(const Operation &op) const { return !(*this == op); }

    [[nodiscard]] bool isValid() const { return jobId != kJobIdDefault; }

    [[nodiscard]] bool isMaintenance() const { return maintId.has_value(); }
};
} // namespace fms::problem

/// @brief Specialization of std::hash for @ref fms::problem::Operation
template <> struct std::hash<fms::problem::Operation> {
    /// @brief Computes the hash of an operation
    /// @param op The operation to compute the hash of
    /// @return The computed hash
    inline std::size_t operator()(const fms::problem::Operation &op) const {
        const std::size_t result = std::hash<fms::problem::JobId>()(op.jobId);
        return fms::algorithms::hash_combine(result, op.operationId);
    }
};

/// @brief Specialization of fmt::formatter for @ref fms::problem::Operation
template <> struct fmt::formatter<fms::problem::Operation> : formatter<std::string_view> {
    /// @brief Formats an operation for output
    /// @param op The operation to format
    /// @param ctx The context to format into
    /// @return The output iterator after formatting
    template <typename FormatContext>
    auto format(const fms::problem::Operation &op, FormatContext &ctx) const -> decltype(ctx.out()) {
        return format_to(ctx.out(), FMT_COMPILE("({}, {})"), op.jobId, op.operationId);
    }
};

#endif // FMS_PROBLEM_OPERATION_HPP
