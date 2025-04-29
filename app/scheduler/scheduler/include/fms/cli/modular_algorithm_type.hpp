#ifndef FMS_CLI_MODULAR_ALGORITHM_TYPE_HPP
#define FMS_CLI_MODULAR_ALGORITHM_TYPE_HPP

namespace fms::cli {

class ModularAlgorithmType {
public:
    enum Value { BROADCAST, COCKTAIL };
    ModularAlgorithmType() = default;

    // NOLINTNEXTLINE: Allow implicit conversion from Value so we can use it as an enum
    constexpr ModularAlgorithmType(Value algorithm) : m_value(algorithm) {}

    // NOLINTNEXTLINE: Allow implicit conversion to Value so we can use it in a switch
    operator Value() const { return m_value; }

    explicit operator bool() = delete;

    [[nodiscard]] static ModularAlgorithmType parse(const std::string &name);

    /**
     * @brief Get the description of the Distributed scheduling algorithm.
     * @return std::string_view Full name of the algorithm.
     */
    [[nodiscard]] std::string_view description() const;

    /**
     * @brief Get the name that is used in the command line to select this algorithm.
     * @return std::string_view Short name of the algorithm.
     */
    [[nodiscard]] std::string_view shortName() const;

    /**
     * @brief Get the full name of the Distributed scheduling algorithm.
     * @return std::string_view Full name of the algorithm.
     */
    [[nodiscard]] std::string_view fullName() const;

    static constexpr std::array allAlgorithms{Value::BROADCAST, Value::COCKTAIL};

private:
    Value m_value;
};
} // namespace fms::cli

#endif // FMS_CLI_MODULAR_ALGORITHM_TYPE_HPP
