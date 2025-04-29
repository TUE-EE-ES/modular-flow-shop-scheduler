#ifndef FMS_CLI_ALGORITHM_TYPE_HPP
#define FMS_CLI_ALGORITHM_TYPE_HPP

namespace fms::cli {
// How to use a class as an Enum with methods, from:
// https://stackoverflow.com/a/53284026/4005637
class AlgorithmType {
public:
    enum Value {
        BHCS,           /// Bounded Heuristic Constraint Scheduler
        MDBHCS,         /// Multi-Dimensional BHCS
        MIBHCS,         /// Maintenance-aware forward heuristic
        MISIM,          /// Simulate maintenance insertion at enf of forward heuristic
        ASAP,           /// As soon as possible scheduling (inserts the operation without ranking)
        ASAP_BACKTRACK, /// ASAP with backtracking
        MIASAP,         /// Maintenance-aware ASAP
        MIASAPSIM,      /// Maintenance-aware ASAP with maintenance as post-scheduling decision
        MNEH,           /// MNEH heuristic (B. Jeong and Y. Kim)
        MNEH_ASAP,      /// MNEH heuristic with ASAP for the initial sequence
        MNEH_ASAP_BACKTRACK, /// MNEH heuristic with ASAP for the initial sequence and backtracking
        MNEH_BHCS_COMBI, /// MNEH heuristic with BHCS for the initial sequence using a combination
                         /// of the parameters
        MNEH_BHCS_FLEXIBLE, /// MNEH heuristic with BHCS for the initial sequence and increased
                            /// flexibility
        MINEH,              /// Maintenance-aware MNEH
        MINEHSIM,           /// Maintenance-aware MNEH with maintenance as post-scheduling decision
        BRANCH_BOUND,       // Branch and bound solver
        GIVEN_SEQUENCE,     /// Use a given sequence to generate a schedule
        ANYTIME,            /// Anytime solver for n-re-entrancy
        ITERATED_GREEDY,    /// Iterated greedy solver for n-re-entrancy
        DD,                 /// Decision diagram solver
        DDSeed,             /// Decision diagram solver built around a seed solution
        SIMPLE              /// Simple scheduler that does not interleave operations
    };

    AlgorithmType() = default;

    // NOLINTNEXTLINE: Allow implicit conversion from Value so we can use it as an enum
    constexpr AlgorithmType(Value algorithm) : m_value(algorithm) {}

    // NOLINTNEXTLINE: Allow implicit conversion to Value so we can use it in a switch
    constexpr operator Value() const { return m_value; }

    explicit operator bool() = delete;

    /**
     * Get the description of what the algorithm does.
     * @return std::string_view Full name of the algorithm.
     */
    [[nodiscard]] std::string_view description() const;

    /**
     * @brief Get the name that is used in the command line to select this algorithm.
     * @return std::string_view Short name of the algorithm.
     */
    [[nodiscard]] std::string_view shortName() const;

    /**
     * @brief Get the full name of the algorithm.
     * @return std::string_view Full name of the algorithm.
     */
    [[nodiscard]] std::string_view fullName() const;

    [[nodiscard]] static AlgorithmType parse(std::string_view name);

    static constexpr std::array allAlgorithms{Value::BHCS,
                                              Value::MDBHCS,
                                              Value::MIBHCS,
                                              Value::MISIM,
                                              Value::ASAP,
                                              Value::ASAP_BACKTRACK,
                                              Value::MIASAP,
                                              Value::MIASAPSIM,
                                              Value::MNEH,
                                              Value::MNEH_ASAP,
                                              Value::MNEH_ASAP_BACKTRACK,
                                              Value::MNEH_BHCS_COMBI,
                                              Value::MNEH_BHCS_FLEXIBLE,
                                              Value::MINEH,
                                              Value::MINEHSIM,
                                              Value::BRANCH_BOUND,
                                              Value::GIVEN_SEQUENCE,
                                              Value::ANYTIME,
                                              Value::ITERATED_GREEDY,
                                              Value::DD,
                                              Value::DDSeed,
                                              Value::SIMPLE};

private:
    Value m_value;
};
} // namespace fms::cli

#endif // FMS_CLI_ALGORITHM_TYPE_HPP
