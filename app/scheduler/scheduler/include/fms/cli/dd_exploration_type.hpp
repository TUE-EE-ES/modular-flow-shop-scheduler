#ifndef FMS_CLI_DD_EXPLORATION_TYPE_HPP
#define FMS_CLI_DD_EXPLORATION_TYPE_HPP

namespace fms::cli {
class DDExplorationType {
public:
    enum Value { BREADTH, DEPTH, BEST, STATIC, ADAPTIVE };

    DDExplorationType() = default;

    // NOLINTNEXTLINE: Allow implicit conversion from Value so we can use it as an enum
    constexpr DDExplorationType(Value explorationtype) : m_value(explorationtype) {}

    // NOLINTNEXTLINE: Allow implicit conversion to Value so we can use it in a switch
    operator Value() const { return m_value; }

    explicit operator bool() = delete;

    [[nodiscard]] static DDExplorationType parse(const std::string &name);

    [[nodiscard]] std::string_view shortName() const;

private:
    Value m_value;
};
} // namespace fms::cli

#endif // FMS_CLI_DD_EXPLORATION_TYPE_HPP
