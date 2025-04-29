#ifndef MULTI_ALGORITHM_BEHAVIOUR_HPP
#define MULTI_ALGORITHM_BEHAVIOUR_HPP

namespace fms::cli {
class MultiAlgorithmBehaviour {
public:
    enum Value { FIRST, LAST, INTERLEAVE, DIVIDE, RANDOM };

    MultiAlgorithmBehaviour() = default;
    constexpr MultiAlgorithmBehaviour(Value behaviour) : m_value(behaviour) {}

    constexpr operator Value() const { return m_value; }

    [[nodiscard]] static MultiAlgorithmBehaviour parse(const std::string &shortName);

    [[nodiscard]] std::string_view shortName() const;

    [[nodiscard]] std::string_view description() const;

    static constexpr std::array allBehaviours{
            Value::FIRST, Value::LAST, Value::INTERLEAVE, Value::DIVIDE, Value::RANDOM};

private:
    Value m_value;
};
} // namespace fms::cli

#endif // MULTI_ALGORITHM_BEHAVIOUR_HPP
