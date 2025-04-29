#ifndef FMS_UTILS_ALTER_XML_HPP
#define FMS_UTILS_ALTER_XML_HPP

#include "fms/delay.hpp"

#include <rapidxml/rapidxml.hpp>
#include <string>

namespace fms::utils {

void insertMaintenanceOperation(const std::string &filenameIn,
                                const std::string &filenameOut,
                                delay pVal);
void insertMaintenanceOperation(const std::string &filenameIn, const std::string &filenameOut);
void writeProcessingTime(rapidxml::xml_document<char> &doc,
                         rapidxml::xml_node<> *pTimes,
                         const char *jobCount,
                         delay pVal);
void addAttribute(rapidxml::xml_document<char> &doc,
                  rapidxml::xml_node<> *node,
                  const char *attr_name,
                  const char *attr_value);
void writeSheetSize(rapidxml::xml_document<char> &doc,
                    rapidxml::xml_node<> *sSizes,
                    const char *jobCount);

} // namespace fms::utils

#endif // FMS_UTILS_ALTER_XML_HPP
