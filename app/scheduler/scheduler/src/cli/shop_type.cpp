#include "fms/pch/containers.hpp"
#include "fms/pch/utils.hpp"

#include "fms/cli/shop_type.hpp"

using namespace fms;
using namespace fms::cli;

ShopType ShopType::parse(const std::string &name) {
    const std::string lowerCase = utils::strings::toLower(name);

    if (lowerCase == "flow") {
        return ShopType::FLOWSHOP;
    }
    if (lowerCase == "job") {
        return ShopType::JOBSHOP;
    }
    if (lowerCase == "fixedorder") {
        return ShopType::FIXEDORDERSHOP;
    }

    throw std::runtime_error("Unknown shop type: " + name);
}

std::string_view ShopType::shortName() const {
    switch (m_value) {
    case Value::FLOWSHOP:
        return "flow";
    case Value::JOBSHOP:
        return "job";
    case Value::FIXEDORDERSHOP:
        return "fixedorder";
    }

    return "";
}
