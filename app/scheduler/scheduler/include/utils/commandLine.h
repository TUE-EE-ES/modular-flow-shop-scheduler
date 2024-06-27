// this file implements a command line parser using getopt()

#ifndef COMMAND_LINE_H_
#define COMMAND_LINE_H_

#include "logger.h"

// How to use a class as an Enum with methods, from:
// https://stackoverflow.com/a/53284026/4005637
class AlgorithmType {
public:
    enum Value {
        BHCS,            /// Bounded Heuristic Constraint Scheduler
        MDBHCS,          /// Multi-Dimensional BHCS
        MIBHCS,          /// Maintenance-aware forward heuristic
        MISIM,           /// Simulate maintenance insertion at enf of forward heuristic
        ASAP,            /// As soon as possible scheduling (inserts the operation without ranking)
        MIASAP,          /// Maintenance-aware ASAP
        MIASAPSIM,       /// Maintenance-aware ASAP with maintenance as post-scheduling decision
        NEH,             /// MNEH heuristic (B. Jeong and Y. Kim)
        MINEH,           /// Maintenance-aware MNEH
        MINEHSIM,        /// Maintenance-aware MNEH with maintenance as post-scheduling decision
        BRANCH_BOUND,    // Branch and bound solver
        GIVEN_SEQUENCE,  /// Use a given sequence to generate a schedule
        ANYTIME,         /// Anytime solver for n-re-entrancy
        ITERATED_GREEDY, /// Iterated greedy solver for n-re-entrancy
        DD,              /// Decision diagram solver
        DDSeed,          /// Decision diagram solver built around a seed solution
        SIMPLE           /// Simple scheduler that does not interleave operations
    };

    AlgorithmType() = default;
    
    // NOLINTNEXTLINE: Allow implicit conversion from Value so we can use it as an enum
    constexpr AlgorithmType(Value algorithm) : m_value(algorithm) {}

    // NOLINTNEXTLINE: Allow implicit conversion to Value so we can use it in a switch
    operator Value() const { return m_value; }

    explicit operator bool() = delete;

    [[nodiscard]] std::string_view fullName() const;

    /**
     * @brief Get the name that is used in the command line to select this algorithm.
     * @return std::string_view Short name of the algorithm.
     */
    [[nodiscard]] std::string_view shortName() const;

    [[nodiscard]] static AlgorithmType parse(std::string_view name);

    static constexpr std::array allAlgorithms{
            Value::BHCS,
            Value::MDBHCS,
            Value::MIBHCS,
            Value::MISIM,
            Value::ASAP,
            Value::MIASAP,
            Value::MIASAPSIM,
            Value::NEH,
            Value::MINEH,
            Value::MINEHSIM,
            Value::BRANCH_BOUND,
            Value::GIVEN_SEQUENCE,
            Value::ANYTIME,
            Value::ITERATED_GREEDY,
            Value::DD,
            Value::SIMPLE
    };

private:
    Value m_value;
};

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
     * @brief Get the full name of the Distributed scheduling algorithm.
     * @return std::string_view Full name of the algorithm.
     */
    [[nodiscard]] std::string_view fullName() const;

    /**
     * @brief Get the name that is used in the command line to select this algorithm.
     * @return std::string_view Short name of the algorithm.
     */
    [[nodiscard]] std::string_view shortName() const;

    static constexpr std::array allAlgorithms{Value::BROADCAST, Value::COCKTAIL};

private:
    Value m_value;
};

class ScheduleOutputFormat {
public:
    enum Value { OPERATION_TIMES, SEPARATION_AND_BUFFER, JSON, CBOR };

    ScheduleOutputFormat() = default;

    // NOLINTNEXTLINE: Allow implicit conversion from Value so we can use it as an enum
    constexpr ScheduleOutputFormat(Value algorithm) : m_value(algorithm) {}

    // NOLINTNEXTLINE: Allow implicit conversion to Value so we can use it in a switch
    operator Value() const { return m_value; }

    explicit operator bool() = delete;

    [[nodiscard]] static ScheduleOutputFormat parse(const std::string &name);

    [[nodiscard]] std::string_view shortName() const;

private:
    Value m_value;
};

class ShopType {
public:
    enum Value { FLOWSHOP, JOBSHOP, FIXEDORDERSHOP};

    ShopType() = default;

    // NOLINTNEXTLINE: Allow implicit conversion from Value so we can use it as an enum
    constexpr ShopType(Value shoptype) : m_value(shoptype) {}

    // NOLINTNEXTLINE: Allow implicit conversion to Value so we can use it in a switch
    operator Value() const { return m_value; }

    explicit operator bool() = delete;

    [[nodiscard]] static ShopType parse(const std::string &name);

    [[nodiscard]] std::string_view shortName() const;

private:
    Value m_value;
};

class DDExplorationType {
public:
    enum Value { BREADTH, DEPTH, BEST, STATIC, ADAPTIVE};

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


struct commandLineArgs{
    // mandatory
    std::string inputFile;
    std::string outputFile;
    std::string sequenceFile;

    // optional - Must be filled with defs.
    // NOLINTBEGIN 
    std::string maintPolicyFile = "";
    LOGGER_LEVEL verbose = LOGGER_LEVEL::CRITICAL;
    double productivityWeight = 0.7;
    double flexibilityWeight = 0.25;
    double tieWeight = 0.05;
    std::chrono::milliseconds timeOut{5000};
    std::uint64_t maxIterations = std::numeric_limits<std::uint64_t>::max();
    std::uint32_t maxPartialSolutions = 5;
    AlgorithmType algorithm = AlgorithmType::BHCS;
    ModularAlgorithmType modularAlgorithm = ModularAlgorithmType::BROADCAST;
    ScheduleOutputFormat outputFormat = ScheduleOutputFormat::OPERATION_TIMES;
    ShopType shopType = ShopType::FIXEDORDERSHOP;
    DDExplorationType explorationType = DDExplorationType::STATIC;
    std::vector<std::string> modularAlgorithmOption;
    // NOLINTEND
};

class commandLine {

public:
    /* function that extracts the command line arguments and fills a struct */
    static commandLineArgs getArgs(int argc, char **argv) noexcept;
};

#endif /* COMMAND_LINE_H_ */
