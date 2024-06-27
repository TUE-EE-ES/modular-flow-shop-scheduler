#ifndef UTILS_TYPES_HPP
#define UTILS_TYPES_HPP

#include <fmt/core.h>

#include <cstdint>
#include <limits>

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

    constexpr StrongType operator++(int) noexcept {
        StrongType copy(value);
        ++value;
        return copy;
    }

    constexpr inline StrongType operator+(StrongType other) const noexcept {
        return StrongType{value + other.value};
    }

    constexpr inline StrongType operator+(ValueType other) const noexcept {
        return StrongType{value + other};
    }

    constexpr inline StrongType operator-(StrongType other) const noexcept {
        return StrongType{value - other.value};
    }

    constexpr inline StrongType operator-(ValueType other) const noexcept {
        return StrongType{value - other};
    }

    auto operator<=>(const StrongType& other) const noexcept = default;

    constexpr static inline StrongType min() noexcept {
        return StrongType{std::numeric_limits<ValueType>::min()};
    }

    constexpr static inline StrongType max() noexcept {
        return StrongType{std::numeric_limits<ValueType>::max()};
    }
};

} // namespace FMS::Utils

// FMT specialization for StrongType
template <typename Tag, typename ValueType>
struct fmt::formatter<FMS::Utils::StrongType<Tag, ValueType>> : formatter<ValueType> {
    template <typename FormatCtx>
    auto format(FMS::Utils::StrongType<Tag, ValueType> t, FormatCtx &ctx) {
        return formatter<ValueType>::format(t.value, ctx);
    }
};

// Hash specialization for StrongType
template <typename Tag, typename ValueType> // NOLINTNEXTLINE(cert-dcl58-cpp)
struct std::hash<FMS::Utils::StrongType<Tag, ValueType>> {
    std::size_t operator()(const FMS::Utils::StrongType<Tag, ValueType> &t) const noexcept {
        return std::hash<ValueType>{}(t.value);
    }
};

// NOLINTBEGIN
#define STRONG_TYPE(name, type)                                                                    \
    struct P_##name##Tag {};                                                                       \
    using name = FMS::Utils::StrongType<P_##name##Tag, type>;                                      \
    static_assert(std::is_integral_v<type>, "StrongType must be an integral type");
// NOLINTEND

#endif // UTILS_TYPES_HPP