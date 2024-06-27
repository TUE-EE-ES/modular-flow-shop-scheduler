#ifndef FORPFSSPSD_OPERATION_H
#define FORPFSSPSD_OPERATION_H

#include "indices.hpp"
#include "utils/hash.hpp"

#include <cstdint>
#include <fmt/compile.h>
#include <istream>
#include <limits>

namespace FORPFSSPSD {
// inner types for temporary storage and mappings

/// Order in which jobs should output the last machine
using JobOutOrder = std::size_t;

/// Index of a maintenance class
using MaintType = std::uint32_t;

struct operation {
    JobId jobId;
    OperationId operationId;
    MaintType maintId;

    bool operator<(const operation &rhs) const {
        return std::tie(jobId, operationId) < std::tie(rhs.jobId, rhs.operationId);
    }

    friend std::istream &operator>>(std::istream &in, operation &op) {
        in.ignore(std::numeric_limits<std::streamsize>::max(), '(');
        JobId::ValueType jobId;
        in >> jobId;
        op.jobId = JobId(jobId);
        in.ignore(std::numeric_limits<std::streamsize>::max(), ',');
        in >> op.operationId;
        in.ignore(std::numeric_limits<std::streamsize>::max(), ')');
        return in;
    }

    bool operator==(const FORPFSSPSD::operation &op) const {
        return op.jobId == this->jobId && op.operationId == this->operationId;
    }
    bool operator!=(const FORPFSSPSD::operation &op) const { return !(*this == op); }
};
} // namespace FORPFSSPSD

template <> struct std::hash<FORPFSSPSD::operation> {
    inline std::size_t operator()(const FORPFSSPSD::operation &op) const {
        const std::size_t result = std::hash<FORPFSSPSD::JobId>()(op.jobId);
        return algorithm::hash_combine(result, op.operationId);
    }
};

template <> struct fmt::formatter<FORPFSSPSD::operation> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const FORPFSSPSD::operation &op, FormatContext &ctx) -> decltype(ctx.out()) {
        return format_to(ctx.out(), FMT_COMPILE("({}, {})"), op.jobId, op.operationId);
    }
};

#endif // FORPFSSPSD_OPERATION_H
