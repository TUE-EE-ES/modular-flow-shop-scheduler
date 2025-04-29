#ifndef FMS_CLI_SHOP_TYPE_HPP
#define FMS_CLI_SHOP_TYPE_HPP

namespace fms::cli {
class ShopType {
public:
    enum Value { FLOWSHOP, JOBSHOP, FIXEDORDERSHOP };

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
} // namespace fms::cli

#endif // FMS_CLI_SHOP_TYPE_HPP
