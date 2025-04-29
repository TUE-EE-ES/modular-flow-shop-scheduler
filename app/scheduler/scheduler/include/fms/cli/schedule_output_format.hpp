#ifndef FMS_CLI_SCHEDULE_OUTPUT_FORMAT_HPP
#define FMS_CLI_SCHEDULE_OUTPUT_FORMAT_HPP

namespace fms::cli {
class ScheduleOutputFormat {
public:
    enum Value { JSON, CBOR };

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
} // namespace fms::cli

#endif // FMS_CLI_SCHEDULE_OUTPUT_FORMAT_HPP
