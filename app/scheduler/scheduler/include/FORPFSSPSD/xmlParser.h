/*@file
 * @author  Umar Waqas <u.waqas@tue.nl>
 * @version 0.1
 *
 * @section LICENSE
 * The licensing of this software is in progress.
 *
 * @section DESCRIPTION
 * scheduleSpecsXmlParser is the class for Schedule spec xml parsing.
 *
 */

#ifndef FORPFSSPSD_XML_PARSER_H_
#define FORPFSSPSD_XML_PARSER_H_

#include "FORPFSSPSD.h"
#include "FORPFSSPSD/indices.hpp"
#include "FORPFSSPSD/production_line.hpp"
#include "rapidxml.hpp"
#include "utils/commandLine.h"
#include "utils/default_map.hpp"
#include "utils/xmlParser.h"

class SingleFlowShopParser {
public:
    SingleFlowShopParser();

    [[nodiscard]] FORPFSSPSD::Instance
    extractInformation(std::string_view fileName, const rapidxml::xml_node<> *root, ShopType type);

    static void loadMaintenancePolicy(FORPFSSPSD::Instance &instance, std::string fname);

private:
    using TimeTable = FMS::TwoKeyMap<FORPFSSPSD::operation, delay>;

    std::uint32_t numberOfJobs;

    /// Array of sheet size indicators. Size must equal to total number of operations.
    std::map<FORPFSSPSD::operation, unsigned int> sheetSizes;

    /// Mapping of a job to an absolute due date
    FORPFSSPSD::JobsTime m_absoluteDueDates;

    /// Mapping of a machine to its module
    std::map<FORPFSSPSD::MachineId, FORPFSSPSD::ModuleId> m_machineToModule;

    /// Listing of jobs and their operations
    FORPFSSPSD::JobOperations m_jobOperationsSet;

    /// Mapping of operations to machines
    FORPFSSPSD::OperationMachineMap m_operationMachineMap;

    // default and maximum values
    delay m_defaultSheetSize, m_maximumSheetSize;

    void loadFlowVector(const rapidxml::xml_node<> *root, const rapidxml::xml_node<> *fvNode);

    void loadFlowVectorV1(const rapidxml::xml_node<> *root,
                          const rapidxml::xml_node<> *flowVectorNode);

    [[nodiscard]] static FORPFSSPSD::DefaultOperationsTime
    loadProcessingTimes(const rapidxml::xml_node<> *pTimes);

    [[nodiscard]] std::tuple<FORPFSSPSD::DefaultTimeBetweenOps, FORPFSSPSD::TimeBetweenOps>
    loadSetupTimes(const rapidxml::xml_node<> *root);

    [[nodiscard]] std::tuple<TimeTable, TimeTable> loadDueDates(const rapidxml::xml_node<> *root);

    void loadSheetSizes(const rapidxml::xml_node<> *sSizes);

    [[nodiscard]] static std::map<FORPFSSPSD::MaintType, delay>
    loadMaintenanceProcessingTimes(const rapidxml::xml_node<> *pTimes);

    [[nodiscard]] static std::map<FORPFSSPSD::MaintType, std::tuple<delay, delay>>
    loadMaintenanceThresholds(const rapidxml::xml_node<> *thresh);

    [[nodiscard]] std::tuple<TimeTable, std::optional<delay>>
    loadTimeTable(const rapidxml::xml_node<> *node, std::string_view subNodes);

    [[nodiscard]] static FORPFSSPSD::PlexityTable loadJobPlexity(const rapidxml::xml_node<> *jp);
};

/**
 * @brief Reads the specification of a Fixed-Order Permutation Flow Shop Sequence-dependent Setup
 * time Scheduling Problem from an XML file.
 */
class FORPFSSPSDXmlParser : public xmlParser {
public:
    enum class FileType {
        /// Indicates that multiple shops (modules) are defined in this file
        MODULAR,

        /// Indicates that a single shop problem is defined in this file
        SHOP,
    };

    explicit FORPFSSPSDXmlParser(std::string fname);
    FORPFSSPSDXmlParser(const FORPFSSPSDXmlParser &) = delete;
    FORPFSSPSDXmlParser(FORPFSSPSDXmlParser &&) = default;
    ~FORPFSSPSDXmlParser() override = default;

    FORPFSSPSDXmlParser &operator=(const FORPFSSPSDXmlParser &) = delete;
    FORPFSSPSDXmlParser &operator=(FORPFSSPSDXmlParser &&) = default;

    /**
     * @brief Loads the XML file and extracts some information. Must be called before any other
     * method.
     */
    void loadXml() override;

    [[nodiscard]] FORPFSSPSD::Instance createFlowShop(ShopType type = ShopType::FIXEDORDERSHOP);

    [[nodiscard]] FORPFSSPSD::ProductionLine
    createProductionLine(ShopType type = ShopType::FIXEDORDERSHOP);

    [[nodiscard]] inline FileType getFileType() const {
        if (!isLoaded()) {
            throw std::runtime_error("XML file not loaded");
        }
        return m_fileType;
    }

    inline static void loadMaintenancePolicy(FORPFSSPSD::Instance &instance, std::string fname) {
        SingleFlowShopParser::loadMaintenancePolicy(instance, std::move(fname));
    }

protected:
    rapidxml::xml_node<> *getFirstNode() {
        if (!isLoaded()) {
            throw std::runtime_error("XML file not loaded");
        }
        return m_nodeRoot;
    }

private:
    FileType m_fileType;

    rapidxml::xml_node<> *m_nodeRoot;

    [[nodiscard]] static FORPFSSPSD::ModulesTransferConstraints loadTransferPoints(
            const rapidxml::xml_node<> *topNode,
            const std::unordered_map<FORPFSSPSD::ModuleId, FORPFSSPSD::Instance> &modules);

    [[nodiscard]] static std::tuple<std::unordered_map<FORPFSSPSD::JobId, delay>,
                                    std::optional<delay>>
    loadJobTimings(const rapidxml::xml_node<> *node,
                   std::string_view subNodes,
                   const FORPFSSPSD::JobOperations &jobOperations);
};

#endif /* FORPFSSPSD_XML_PARSER_H_ */
