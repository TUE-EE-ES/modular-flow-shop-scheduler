/*
 * Author		: Umar Waqas
 * Date			: 10-07-2013
 * Email		: u.waqas@tue.nl

 * License notice:
 * The licensing of this software is in progress.
 *
 * Description	:
 * This file contains the implementations of a
 * wrapper around the RapidXML API
 */

#include "pch/containers.hpp"

#include "utils/xmlParser.h"

#include "fmsschedulerexception.h"

#include <exception>
#include <filesystem>
#include <fmt/core.h>

using namespace rapidxml;

/* parametrized constructor for xmlParser
 * @param fname - Name of the file containing the xml
 * */
xmlParser::xmlParser(std::string fname) :
    m_filename(std::move(fname)),
    m_loaded(false),
    m_doc(std::make_unique<rapidxml::xml_document<>>()) {}

/* loads an xml file
 * @param fname - Name of the file containing the xml
 * */
void xmlParser::loadXml() {
    if (m_loaded) {
        return;
    }
    try {
        m_xml = file<>(m_filename.c_str()).data();
        m_doc->parse<parse_full>(m_xml.data());
        m_loaded = true;
    } catch (std::exception &e) {
        throw FmsSchedulerException(
                fmt::format("xmlParser failed to load xml file: {}. Current path: {}. {}",
                            m_filename,
                            std::filesystem::current_path().string(),
                            e.what()));
    }
}
/* Serializes the filename to the ostream
 * @param output - oStream to write to
 * @param p - xmlParser to serialize
 * */
std::ostream &operator<<(std::ostream &output, const xmlParser &p) {
    output << "Filename: " << p.m_filename << std::endl;
    return output;
}
