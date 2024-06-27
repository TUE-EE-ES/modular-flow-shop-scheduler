#ifndef PLEXITY_HPP
#define PLEXITY_HPP

#include <cstdint>
#include <string>

namespace FORPFSSPSD {

// How to use a class as an Enum with methods, from:
// https://stackoverflow.com/a/53284026/4005637
class Plexity {
public:
    enum Value { SIMPLEX = 1, DUPLEX = 2 };

    Plexity() = default;

    explicit Plexity(const std::string &s);

    // NOLINTNEXTLINE: Allow implicit conversion from Value so we can use it as an enum
    constexpr Plexity(Value value) : m_value(value) {}

    // NOLINTNEXTLINE: Allow implicit conversion to Value so we can use it in a switch
    operator Value() const { return m_value; }

    bool operator==(const Plexity &other) const { return m_value == other.m_value; }
    bool operator!=(const Plexity &other) const { return m_value != other.m_value; }

    bool operator==(const Plexity::Value &value) const { return m_value == value; }
    bool operator!=(const Plexity::Value &value) const { return m_value != value; }

    operator bool() = delete;

    [[nodiscard]] std::uint32_t numberOfOps() const;

    [[nodiscard]] static std::uint32_t maxOps() { return 2; }

private:
    Value m_value;
};

} // namespace FORPFSSPSD

#endif // PLEXITY_HPP