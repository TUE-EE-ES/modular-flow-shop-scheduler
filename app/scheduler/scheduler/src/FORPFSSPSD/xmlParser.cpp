/*
 * Author		: Umar Waqas
 * Date			: 10-07-2013
 * Email		: u.waqas@tue.nl

 * License notice:
 * The licensing of this software is in progress.
 *
 * Description	:
 * This file contains the implementations of a
 * Initialization xml parser
 */

#include "pch/containers.hpp"
#include "pch/utils.hpp"

#include "FORPFSSPSD/xmlParser.h"

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "FORPFSSPSD/indices.hpp"
#include "FORPFSSPSD/operation.h"
#include "FORPFSSPSD/production_line.hpp"
#include "fmsschedulerexception.h"
#include "rapidxml.hpp"

#include <filesystem>
#include <fmt/compile.h>
#include <gsl/narrow>
#include <optional>
#include <string>

namespace fs = std::filesystem;
using namespace FORPFSSPSD;
using namespace FMS;
using namespace rapidxml;

namespace {

std::int64_t intAttribute(const xml_node<> *node, std::string_view name) {
    if (xml_attribute<> *attr = node->first_attribute(name.data(), name.size())) {
        return std::stoll(attr->value());
    }
    throw ParseException(
            fmt::format("No attribute '{}' found for node named '{}' \n", name, node->name()));
}

delay getRequiredDefault(const std::optional<delay> defVal) {
    if (defVal) {
        return defVal.value();
    }
    throw ParseException("No default value found for required attribute");
}

std::optional<delay> parseDefault(const xml_node<> *node) {
    const auto *attr = node->first_attribute("default");
    if (attr == nullptr) {
        return std::nullopt;
    }

    char *pEnd = nullptr;
    const auto val = std::strtoll(attr->value(), &pEnd, 10);
    if (pEnd == attr->value()) {
        LOG(LOGGER_LEVEL::WARNING,
            "Unrecognized default value '{}' for node '{}'",
            attr->value(),
            node->name());
    }
    return val;
}

rapidxml::xml_node<> *findRequired(const xml_node<> *node, std::string_view name) {
    xml_node<> *result = node->first_node(name.data(), name.size());
    if (result == nullptr) {
        throw ParseException(fmt::format("Expected a '{}' node", name));
    }
    return result;
}

rapidxml::xml_attribute<> *findRequiredAttribute(const xml_node<> *topNode,
                                                 std::string_view attributeName) {
    auto *attribute = topNode->first_attribute(attributeName.data(), attributeName.size(), false);
    if (attribute == nullptr) {
        throw ParseException(fmt::format("Expected a '{}' attribute", attributeName));
    }
    return attribute;
}

} // namespace

SingleFlowShopParser::SingleFlowShopParser() :
    numberOfJobs(-1), m_defaultSheetSize(-1), m_maximumSheetSize(-1) {}

FORPFSSPSD::Instance SingleFlowShopParser::extractInformation(std::string_view fileName,
                                                              const xml_node<> *const root,
                                                              const ShopType type) {

    // get the jobs node
    const auto *nodeJobs = findRequired(root, "jobs");
    const auto *attrJobCount = findRequiredAttribute(nodeJobs, "count");
    numberOfJobs = std::stoi(attrJobCount->value());

    LOG("SingleFlowshopParser: Number of Jobs: {} \n", numberOfJobs);

    loadFlowVector(root, findRequired(root, "flowVector"));

    auto processingTimes = loadProcessingTimes(findRequired(root, "processingTimes"));
    loadSheetSizes(root->first_node("sizes"));
    auto [setupTimes, setupTimesIndep] = loadSetupTimes(root);
    auto [dueDates, dueDatesIndep] = loadDueDates(root);

    return {fs::path(fileName).stem().string(),
            std::move(m_jobOperationsSet),
            std::move(m_operationMachineMap),
            std::move(processingTimes),
            std::move(setupTimes),
            std::move(setupTimesIndep),
            std::move(dueDates),
            std::move(dueDatesIndep),
            std::move(m_absoluteDueDates),
            std::move(sheetSizes),
            m_defaultSheetSize,
            m_maximumSheetSize,
            type};
}

void SingleFlowShopParser::loadMaintenancePolicy(FORPFSSPSD::Instance &instance,
                                                 std::string fname) {
    /* assign the file name to the xml parser */
    xmlParser maintParser(std::move(fname));
    maintParser.loadXml();

    /* parse the xml and load */
    const auto *const maintPolicy = maintParser.getFirstNode("MaintenanceActions");
    if (maintPolicy == nullptr) {
        throw ParseException("Expected a Maintenance Actions root element!");
    }

    /* load the variables*/
    const auto *const types = findRequired(maintPolicy, "types");
    const unsigned int numberOfTypes = intAttribute(types, "count");

    const auto *const minIdle = findRequired(maintPolicy, "minimumIdle");
    const auto minimumIdle = static_cast<delay>(intAttribute(minIdle, "value"));

    const auto *const pTimes = findRequired(maintPolicy, "processingTimes");
    const auto defaultMaintDuration = static_cast<delay>(intAttribute(pTimes, "default"));
    auto maintDuration = loadMaintenanceProcessingTimes(pTimes);

    const auto *const thresh = findRequired(maintPolicy, "thresholds");
    const auto defaultThreshold = std::numeric_limits<delay>::max();
    auto thresholds = loadMaintenanceThresholds(thresh);

    /* assign variables to policy of FORPFSSPSD instance */
    instance.setMaintenancePolicy(numberOfTypes,
                                  minimumIdle,
                                  std::move(maintDuration),
                                  defaultMaintDuration,
                                  std::move(thresholds),
                                  defaultThreshold);
}

void SingleFlowShopParser::loadFlowVector(const xml_node<> *const root, const xml_node<> *fvNode) {
    // Check the type of flow vector that we are reading. It is possible that (1) we have an entry
    // for every operation or (2) a general entry for all operations. The difference is that (1)
    // has a "job" attribute and (2) does not.
    const auto *child = fvNode->first_node();
    if (child == nullptr) {
        throw ParseException("Expected at least one flow vector entry");
    }

    if (child->first_attribute("job") == nullptr) {
        loadFlowVectorV1(root, fvNode);
        return;
    }

    for (const auto *child = fvNode->first_node(); child != nullptr;
         child = child->next_sibling()) {
        const auto opId = static_cast<OperationId>(intAttribute(child, "index"));
        const auto mId = static_cast<MachineId>(intAttribute(child, "value"));
        const auto jobId = static_cast<JobId>(intAttribute(child, "job"));

        const operation op{jobId, opId};
        m_jobOperationsSet[jobId].push_back(op);
        m_operationMachineMap[op] = mId;
    }
}

void SingleFlowShopParser::loadFlowVectorV1(const xml_node<> *const root,
                                            const xml_node<> *const flowVectorNode) {
    // Iterate over the flow vector and get the components
    std::vector<std::pair<OperationId, MachineId>> flowVector;
    std::unordered_map<MachineId, ReEntrantId> machinesReEntrantId;
    std::unordered_map<MachineId, ReEntrancies> machinesReEntrancies;
    ReEntrantId currentReEntrantId(0);
    for (const auto *child = flowVectorNode->first_node(); child != nullptr;
         child = child->next_sibling()) {
        const OperationId index = intAttribute(child, "index");
        const auto value = static_cast<MachineId>(intAttribute(child, "value"));
        flowVector.emplace_back(index, value);

        const auto it = machinesReEntrancies.find(value);
        if (it == machinesReEntrancies.end()) {
            machinesReEntrancies.emplace(value, 1);
        } else {
            machinesReEntrantId.emplace(value, currentReEntrantId);
            ++currentReEntrantId;
            ++it->second;
        }
    }

    auto jobPlexity = loadJobPlexity(root->first_node("jobPlexity"));

    // Create operations following the flow vector and the re-entrancies that we know
    for (std::size_t i = 0; i < numberOfJobs; ++i) {
        const auto jobId = static_cast<JobId>(i);
        std::unordered_map<MachineId, std::size_t> reEntrantCount;
        OperationsVector jobOps;

        // Get plexity of the job
        const auto it = jobPlexity.find(jobId);
        std::vector<ReEntrancies> reEntrancies;
        if (it != jobPlexity.end()) {
            reEntrancies = it->second;
        } else {
            reEntrancies = std::vector<ReEntrancies>(machinesReEntrantId.size(), ReEntrancies(2));
        }

        for (const auto &[opId, mId] : flowVector) {
            // Find if we are in a re-entrant machine

            const auto itId = machinesReEntrantId.find(mId);
            if (itId != machinesReEntrantId.end()) {
                // Yes we are in a re-entrant machine. We need to check if we must skip this
                // operation

                const auto jobMachineReEntrancies = reEntrancies.at(itId->second.value);
                const auto machineReEntrancies = machinesReEntrancies.at(mId);

                // Post-increment so 'doneReEntrancies' contains the previous value
                const auto doneReEntrancies = reEntrantCount[mId]++;
                const auto opsToSkip = machineReEntrancies.value - jobMachineReEntrancies.value;

                if (doneReEntrancies < opsToSkip) {
                    // We must skip this operation
                    continue;
                }
            }

            const operation op{jobId, opId};
            jobOps.push_back(op);
            m_operationMachineMap.emplace(op, mId);
        }
        m_jobOperationsSet.emplace(jobId, std::move(jobOps));
    }
}

FORPFSSPSD::DefaultOperationsTime
SingleFlowShopParser::loadProcessingTimes(const xml_node<> *pTimes) {

    // load the default value
    const auto defaultProcessingTime = intAttribute(pTimes, "default");
    DefaultOperationsTime::Table processingTimes;

    // iterate over the processing times
    for (xml_node<> *child = pTimes->first_node(); child != nullptr;
         child = child->next_sibling()) {

        const auto jId = static_cast<JobId>(intAttribute(child, "j"));
        const auto oId = static_cast<OperationId>(intAttribute(child, "op"));
        const auto value = static_cast<delay>(intAttribute(child, "value"));
        processingTimes[FORPFSSPSD::operation{jId, oId}] = value;
    }

    LOG("Loading of the processing times is complete \n");
    return {std::move(processingTimes), defaultProcessingTime};
}

std::tuple<FORPFSSPSD::DefaultTimeBetweenOps, FORPFSSPSD::TimeBetweenOps>
SingleFlowShopParser::loadSetupTimes(const xml_node<> *root) {
    LOG("Loading of the setup times started");

    // load the default value
    auto [tSetupTime, tDefault] = loadTimeTable(findRequired(root, "setupTimes"), "s");
    const auto defaultSetupTime = getRequiredDefault(tDefault);

    FORPFSSPSD::DefaultTimeBetweenOps setupTimes{defaultSetupTime};
    FORPFSSPSD::TimeBetweenOps setupTimesIndep;

    // Check if setupTimesIndependent exists
    const auto *sTimesIndep = root->first_node("setupTimesIndependent");
    if (sTimesIndep != nullptr) {
        setupTimesIndep = std::get<0>(loadTimeTable(sTimesIndep, "s"));
    }

    // Divide between dependent and independent
    for (const auto &[op1, map1] : tSetupTime) {
        for (const auto &[op2, value] : map1) {
            // Check if both operations are from different jobs but the same machine
            if (op1.jobId != op2.jobId
                && m_operationMachineMap.at(op1) == m_operationMachineMap.at(op2)) {
                // It is a dependent setup time
                setupTimes.insert(op1, op2, value);
            } else {
                // It is an independent setup time
                setupTimesIndep.insert(op1, op2, value);
            }
        }
    }

    return {std::move(setupTimes), std::move(setupTimesIndep)};
}

std::tuple<SingleFlowShopParser::TimeTable, SingleFlowShopParser::TimeTable>
SingleFlowShopParser::loadDueDates(const xml_node<> *root) {
    LOG("Loading of the due dates has started");

    TimeTable dueDates;
    TimeTable dueDatesIndep;
    bool check = false;

    const auto *nodeDueDatesIndep = root->first_node("relativeDueDatesIndependent");
    if (nodeDueDatesIndep != nullptr) {
        dueDatesIndep = std::get<0>(loadTimeTable(nodeDueDatesIndep, "d"));
        check = true;
    }

    const auto *nodeDueDates = root->first_node("relativeDueDates");
    if (nodeDueDates != nullptr) {
        dueDates = std::get<0>(loadTimeTable(nodeDueDates, "d"));
    }

    // Divide between dependent and independent
    for (const auto &[op1, map1] : dueDates) {
        for (const auto &[op2, value] : map1) {
            // Check if both operations are from different jobs but the same machine
            if (op1.jobId != op2.jobId
                && m_operationMachineMap.at(op1) == m_operationMachineMap.at(op2)) {
                // It is a dependent due date
                dueDates.insert(op1, op2, value);
            } else {
                if (check) {
                    throw ParseException(fmt::format("\"relativeDueDates\" contains an independent "
                                                     "due date between operations {} and {}",
                                                     op1,
                                                     op2));
                }
                // It is an independent due date
                dueDatesIndep.insert(op1, op2, value);
            }
        }
    }

    //also load absolute due dates
    const auto *dueDatesAbsolute = root->first_node("absoluteDeadlines");
    if (dueDatesAbsolute != nullptr){
        for (xml_node<> *child = dueDatesAbsolute->first_node(); child != nullptr;
            child = child->next_sibling()) {

            const auto jId = static_cast<JobId>(intAttribute(child, "j"));
            const auto value = static_cast<delay>(intAttribute(child, "value"));
            m_absoluteDueDates[jId] = value;
        }
    }

    return {std::move(dueDates), std::move(dueDatesIndep)};
}

void SingleFlowShopParser::loadSheetSizes(const xml_node<> *sSizes) {
    m_maximumSheetSize = 0;
    // make provisions for specifications without sheet size
    if (sSizes == nullptr) {
        // if no entry was given, problem is assumed to not need sheet size
        m_defaultSheetSize = 0;
        return;
    }

    // load the default value
    if (xml_attribute<> *attr = sSizes->first_attribute("default")) {
        m_defaultSheetSize = std::stol(attr->value());
    } else {
        throw ParseException("unable to find default value for sheet sizes \n");
    }

    // iterate over the processing times and sizes
    for (const auto *child = sSizes->first_node(); child != nullptr;
         child = child->next_sibling()) {
        const auto jid = static_cast<JobId>(intAttribute(child, "j"));
        const auto oid = static_cast<OperationId>(intAttribute(child, "op"));
        const auto value = static_cast<delay>(intAttribute(child, "value"));

        sheetSizes[FORPFSSPSD::operation{jid, oid}] = value;
        if (value > m_maximumSheetSize) {
            m_maximumSheetSize = value;
        }
    }

    LOG("Loading of sheet sizes is complete \n");
}

std::map<FORPFSSPSD::MaintType, delay>
SingleFlowShopParser::loadMaintenanceProcessingTimes(const xml_node<> *const pTimes) {
    std::map<FORPFSSPSD::MaintType, delay> maintDuration;

    // iterate over the processing times and sizes
    for (const auto *child = pTimes->first_node(); child != nullptr;
         child = child->next_sibling()) {

        const auto maintType = static_cast<FORPFSSPSD::MaintType>(intAttribute(child, "t"));
        const auto value = static_cast<delay>(intAttribute(child, "value"));
        maintDuration[maintType] = value;
    }

    LOG("Loading of maintenance processing times is complete");
    return maintDuration;
}

std::map<FORPFSSPSD::MaintType, std::tuple<delay, delay>>
SingleFlowShopParser::loadMaintenanceThresholds(const xml_node<> *const thresh) {
    std::map<FORPFSSPSD::MaintType, std::tuple<delay, delay>> thresholds;

    // iterate over the processing times and sizes
    for (const auto *child = thresh->first_node(); child != nullptr;
         child = child->next_sibling()) {

        const auto maintType = static_cast<FORPFSSPSD::MaintType>(intAttribute(child, "t"));
        const auto start = static_cast<delay>(intAttribute(child, "s"));

        delay end = std::numeric_limits<delay>::max();
        if (strcmp(child->first_attribute("e")->value(), "inf") != 0) {
            end = static_cast<delay>(intAttribute(child, "e"));
        }
        thresholds[maintType] = {start, end};
    }

    LOG("Loading of maintenance thresholds is complete");
    return thresholds;
}

std::tuple<SingleFlowShopParser::TimeTable, std::optional<delay>>
SingleFlowShopParser::loadTimeTable(const xml_node<> *const node, std::string_view subNodes) {
    const std::optional<delay> defaultDelay = parseDefault(node);

    TimeTable result;
    for (const auto *child = node->first_node(subNodes.data(), subNodes.size()); child != nullptr;
         child = child->next_sibling(subNodes.data(), subNodes.size())) {
        const auto jid1 = static_cast<JobId>(intAttribute(child, "j1"));
        const auto jid2 = static_cast<JobId>(intAttribute(child, "j2"));
        const auto oid1 = static_cast<OperationId>(intAttribute(child, "op1"));
        const auto oid2 = static_cast<OperationId>(intAttribute(child, "op2"));
        const auto value = static_cast<delay>(intAttribute(child, "value"));
        FORPFSSPSD::operation op1{jid1, oid1};
        FORPFSSPSD::operation op2{jid2, oid2};

        // Sanity check to see if the operations exist
        if (m_operationMachineMap.find(op1) == m_operationMachineMap.end()) {
            throw ParseException(fmt::format("Operation {} does not exist", op1));
        }

        if (m_operationMachineMap.find(op2) == m_operationMachineMap.end()) {
            throw ParseException(fmt::format("Operation {} does not exist", op2));
        }

        // get the mapping from this operation, and if it does not exist, create it
        if (result.contains(op1, op2)) {
            throw ParseException("Table value encountered twice");
        }
        result.insert(op1, op2, static_cast<delay>(value));
    }
    return {std::move(result), defaultDelay};
}

PlexityTable SingleFlowShopParser::loadJobPlexity(const xml_node<> *const jp) {
    FORPFSSPSD::PlexityTable jobPlexity;

    if (jp == nullptr) {
        // if no entry was given, it is assumed to be duplex
        return {};
    }

    for (xml_node<> *child = jp->first_node(); child != nullptr; child = child->next_sibling()) {
        const auto *type = findRequiredAttribute(child, "type");
        FORPFSSPSD::ReEntrancies reEntrancies(2);
        const std::string value(type->value());

        if (value == "S") {
            reEntrancies = ReEntrancies(1);
        } else if (value == "D") {
            reEntrancies = ReEntrancies(2);
        } else {
            // Try to convert it to an integer and if it fails throw an exception
            try {
                reEntrancies = FORPFSSPSD::ReEntrancies(std::stoi(value));
            } catch (std::invalid_argument &e) {
                throw FmsSchedulerException("Found a job plexity entry with an invalid reentrancy");
            }
        }

        // If a value is specified multiple times we assume that there are multiple re-entrant
        // machines and that the order in which they are declared respects the order of the job
        // in the re-entrant machines. Alternatively we could provide an attribute "index"
        const auto jobId = static_cast<FORPFSSPSD::JobId>(intAttribute(child, "j"));
        jobPlexity[jobId].emplace_back(reEntrancies);
    }

    return jobPlexity;
}

FORPFSSPSDXmlParser::FORPFSSPSDXmlParser(std::string fname) :
    xmlParser(std::move(fname)), m_fileType(FileType::SHOP), m_nodeRoot(nullptr) {
    loadXml();
}

FORPFSSPSD::Instance FORPFSSPSDXmlParser::createFlowShop(const ShopType type) {
    SingleFlowShopParser p;
    return p.extractInformation(getFileName(), getFirstNode(), type);
}

FORPFSSPSD::ProductionLine FORPFSSPSDXmlParser::createProductionLine(const ShopType type) {
    const auto *const nodeRoot = getFirstNode();

    std::unordered_map<ModuleId, FORPFSSPSD::Instance> modules;
    for (const auto *node = m_nodeRoot->first_node("SPInstance"); node != nullptr;
         node = node->next_sibling("SPInstance")) {

        const auto moduleId = static_cast<ModuleId>(intAttribute(node, "id"));
        SingleFlowShopParser p;
        modules.emplace(moduleId, p.extractInformation(getFileName(), node, type));
    }
    auto transfer = loadTransferPoints(m_nodeRoot, modules);
    return ProductionLine::fromFlowShops(
            fs::path(getFileName()).stem().string(), modules, transfer);
}

FORPFSSPSD::ModulesTransferConstraints FORPFSSPSDXmlParser::loadTransferPoints(
        const xml_node<> *const topNode,
        const std::unordered_map<FORPFSSPSD::ModuleId, FORPFSSPSD::Instance> &modules) {
    ModulesTransferConstraints result;

    const auto *const transferPoints = findRequired(topNode, "transfers");

    for (const auto *modulesTransfer = transferPoints->first_node("modulesTransfer");
         modulesTransfer != nullptr;
         modulesTransfer = modulesTransfer->next_sibling("modulesTransfer")) {

        const auto idFrom = static_cast<ModuleId>(intAttribute(modulesTransfer, "id_from"));
        const auto idTo = static_cast<ModuleId>(intAttribute(modulesTransfer, "id_to"));

        // Check that the ids are consecutive
        if (idFrom + 1 != idTo) {
            throw ParseException("Ids of transfers between modules must be consecutive");
        }

        const auto itFrom = modules.find(idFrom);
        if (itFrom == modules.end()) {
            throw ParseException(fmt::format("Transfer points: Module {} does not exist", idFrom));
        }

        const auto &jobs = itFrom->second.jobs();

        // Get setup times
        const auto *const nodeSetup = findRequired(modulesTransfer, "setupTimes");
        auto [tSetupTimes, tSetupTimesDef] = loadJobTimings(nodeSetup, "s", jobs);

        // Get due dates
        const auto *const nodeDueDates = findRequired(modulesTransfer, "relativeDueDates");
        auto [tDueDates, _] = loadJobTimings(nodeDueDates, "d", jobs);

        // We need to add the processing times to the due dates to make them correct.
        for (const auto &[jobId, dueDate] : tDueDates) {
            const auto &opLast = jobs.at(jobId).back();
            tDueDates[jobId] = dueDate + itFrom->second.getProcessingTime(opLast);
        }

        result.insert(idFrom,
                      idTo,
                      {{std::move(tSetupTimes), tSetupTimesDef.value_or(0)}, std::move(tDueDates)});
    }

    return result;
}

void FORPFSSPSDXmlParser::loadXml() {
    if (isLoaded()) {
        return;
    }
    xmlParser::loadXml();

    // Check if the root node is an SPInstance
    m_nodeRoot = xmlParser::getFirstNode("SPInstance");
    if (m_nodeRoot == nullptr) {
        throw ParseException("Expected an SPInstance root element!");
    }

    // Check the type of the problem
    const auto *typeAttr = findRequiredAttribute(m_nodeRoot, "type");
    std::string typeStr(typeAttr->value());
    for (auto &c : typeStr) {
        c = static_cast<std::string::value_type>(std::toupper(c));
    }

    if (typeStr == "MODULAR") {
        m_fileType = FileType::MODULAR;
    } else if (typeStr == "FORPFSSPSD") {
        m_fileType = FileType::SHOP;
    } else {
        throw FmsSchedulerException(fmt::format("Unknown type '{}' for SPInstance", typeStr));
    }
}

std::tuple<std::unordered_map<FORPFSSPSD::JobId, delay>, std::optional<delay>>
FORPFSSPSDXmlParser::loadJobTimings(const xml_node<> *node,
                                    std::string_view subNodes,
                                    const JobOperations &jobOperations) {
    const std::optional<delay> defaultDelay = parseDefault(node);

    std::unordered_map<JobId, delay> result;
    for (const auto *child = node->first_node(subNodes.data(), subNodes.size()); child != nullptr;
         child = child->next_sibling(subNodes.data(), subNodes.size())) {
        const auto jid = static_cast<JobId>(intAttribute(child, "j"));
        const auto value = static_cast<delay>(intAttribute(child, "value"));

        if (jobOperations.find(jid) == jobOperations.end()) {
            throw ParseException(fmt::format("Job {} does not exist", jid));
        }

        result.emplace(jid, value);
    }
    return {std::move(result), defaultDelay};
}
