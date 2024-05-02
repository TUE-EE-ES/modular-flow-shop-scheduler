#ifndef UTILS_TYPES_HPP
#define UTILS_TYPES_HPP

#include <cinttypes>
#include <fmt/core.h>

namespace FMS::Utils {

template <typename Tag, typename TValueType = std::uint64_t> struct StrongType {
    using ValueType = TValueType;
    
    ValueType value;

    constexpr StrongType() noexcept = default;

    explicit constexpr StrongType(ValueType value) noexcept : value(value) {}

    explicit constexpr operator ValueType() const noexcept { return value; }

    constexpr StrongType operator++() noexcept {
        ++value;
        return *this;
    }

    constexpr StrongType operator+(StrongType other) const noexcept {
        return StrongType{value + other.value};
    }

    constexpr StrongType operator+(ValueType other) const noexcept {
        return StrongType{value + other};
    }

    auto operator<=>(const StrongType& other) const noexcept = default;
};

} // namespace FMS::Utils

// FMT specialization for StrongType
template <typename Tag, typename ValueType>
struct fmt::formatter<FMS::Utils::StrongType<Tag, ValueType>> : formatter<uint32_t> {
    template <typename FormatCtx>
    auto format(FMS::Utils::StrongType<Tag, ValueType> t, FormatCtx &ctx) {
        return formatter<uint32_t>::format(t.value, ctx);
    }
};

// Hash specialization for StrongType
template <typename Tag, typename ValueType> // NOLINTNEXTLINE(cert-dcl58-cpp)
struct std::hash<FMS::Utils::StrongType<Tag, ValueType>> {
    std::size_t operator()(const FMS::Utils::StrongType<Tag, ValueType> &t) const noexcept {
        return std::hash<ValueType>{}(t.value);
    }
};

#endif // UTILS_TYPES_HPP