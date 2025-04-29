#include "fms/pch/containers.hpp"
#include "fms/pch/fmt.hpp"

#include "fms/utils/alter_xml.hpp"

#include "fms/scheduler_exception.hpp"

#include <filesystem>
#include <fstream>
#include <rapidxml/rapidxml_print.hpp>
#include <rapidxml/rapidxml_utils.hpp>

namespace fs = std::filesystem;
using namespace rapidxml;

namespace fms::utils {

void writeProcessingTime(xml_document<char> &doc,
                         xml_node<> *pTimes,
                         const char *jobCount,
                         delay pVal) {
    xml_node<> *newP = doc.allocate_node(node_element, "p");

    newP = doc.allocate_node(node_element, "p");
    pTimes->append_node(newP);
    addAttribute(doc, newP, "j", jobCount);
    addAttribute(doc, newP, "op", "0");
    addAttribute(doc, newP, "value", "0");

    newP = doc.allocate_node(node_element, "p");
    pTimes->append_node(newP);
    addAttribute(doc, newP, "j", jobCount);
    addAttribute(doc, newP, "op", "1");
    const char *strPVal = doc.allocate_string(std::to_string(pVal).c_str());
    addAttribute(doc, newP, "value", strPVal);

    newP = doc.allocate_node(node_element, "p");
    pTimes->append_node(newP);
    addAttribute(doc, newP, "j", jobCount);
    addAttribute(doc, newP, "op", "2");
    addAttribute(doc, newP, "value", "0");

    newP = doc.allocate_node(node_element, "p");
    pTimes->append_node(newP);
    addAttribute(doc, newP, "j", jobCount);
    addAttribute(doc, newP, "op", "3");
    addAttribute(doc, newP, "value", "0");
}

void addAttribute(xml_document<char> &doc,
                  xml_node<> *node,
                  const char *attr_name,
                  const char *attr_value) {
    xml_attribute<> *attr = doc.allocate_attribute(attr_name, attr_value);
    node->append_attribute(attr);
}

void insertMaintenanceOperation(const std::string &filenameIn,
                                const std::string &filenameOut,
                                delay pVal) {

    /* assign the file name to the xml parser */

    xml_document<char> doc;
    std::string xml;
    try {
        file<> file(filenameIn.c_str());
        xml = file.data();
        xml.push_back('\0');
        doc.parse<parse_full>(xml.data());
    } catch (...) {
        throw FmsSchedulerException(
                fmt::format("xmlParser failed to load xml file: {}.", filenameIn));
    }

    /* parse the xml and load */
    xml_node<> *spInstance = doc.first_node("SPInstance");

    // update job count
    xml_node<> *jobs = spInstance->first_node("jobs");

    // add processing time
    xml_node<> *pTimes = spInstance->first_node("processingTimes");
    writeProcessingTime(doc, pTimes, jobs->first_attribute("count")->value(), pVal);

    // add sheet size
    xml_node<> *sSizes = spInstance->first_node("sizes");
    writeSheetSize(doc, sSizes, jobs->first_attribute("count")->value());

    // //update job count
    auto jobCount = std::stoi(jobs->first_attribute("count")->value());
    ++jobCount;
    const char *strCount = doc.allocate_string(std::to_string(jobCount).c_str());
    jobs->first_attribute("count")->value(strCount);

    // print to file
    std::ofstream fileStored(filenameOut);
    fileStored << doc;
    fileStored.close();
    doc.clear();
}

void insertMaintenanceOperation(const std::string &filenameIn, const std::string &filenameOut) {
    // function just writes to new file
    fs::copy(filenameIn, filenameOut, fs::copy_options::overwrite_existing);
}

void writeSheetSize(xml_document<char> &doc, xml_node<> *sSizes, const char *jobCount) {
    xml_node<> *newZ = doc.allocate_node(node_element, "z");
    newZ = doc.allocate_node(node_element, "z");
    sSizes->append_node(newZ);
    addAttribute(doc, newZ, "j", jobCount);
    addAttribute(doc, newZ, "op", "0");
    addAttribute(doc, newZ, "value", "inf");

    newZ = doc.allocate_node(node_element, "z");
    sSizes->append_node(newZ);
    addAttribute(doc, newZ, "j", jobCount);
    addAttribute(doc, newZ, "op", "1");
    addAttribute(doc, newZ, "value", "inf");

    newZ = doc.allocate_node(node_element, "z");
    sSizes->append_node(newZ);
    addAttribute(doc, newZ, "j", jobCount);
    addAttribute(doc, newZ, "op", "2");
    addAttribute(doc, newZ, "value", "inf");

    newZ = doc.allocate_node(node_element, "z");
    sSizes->append_node(newZ);
    addAttribute(doc, newZ, "j", jobCount);
    addAttribute(doc, newZ, "op", "3");
    addAttribute(doc, newZ, "value", "inf");
}

} // namespace fms::utils
