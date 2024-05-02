#ifndef ALTERXML_H
#define ALTERXML_H

#include "pch/containers.hpp"

#include <rapidxml.hpp>

#include "delay.h"

void insertMaintenanceOperation(const std::string &filenameIn,
                                const std::string &filenameOut,
                                delay pVal);
void insertMaintenanceOperation(const std::string &filenameIn, const std::string &filenameOut);
void writeProcessingTime(rapidxml::xml_document<char> &doc,
                         rapidxml::xml_node<> *pTimes,
                         const char *jobCount,
                         delay pVal);
void addAttribute(rapidxml::xml_document<char>& doc, rapidxml::xml_node<> *node, const char* attr_name, const char* attr_value );
void writeSheetSize(rapidxml::xml_document<char>& doc, rapidxml::xml_node<> *sSizes, const char * jobCount);

#endif // ALTERXML_H