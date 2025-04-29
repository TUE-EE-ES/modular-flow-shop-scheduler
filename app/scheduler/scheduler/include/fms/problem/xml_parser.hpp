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

#ifndef FMS_PROBLEM_XML_PARSER_HPP
#define FMS_PROBLEM_XML_PARSER_HPP

#include "flow_shop.hpp"
#include "indices.hpp"
#include "production_line.hpp"

#include "fms/cli/command_line.hpp"
#include "fms/utils/default_map.hpp"
#include "fms/utils/xml_parser.hpp"

#include <rapidxml/rapidxml.hpp>
#include <string_view>

namespace fms::problem {

class SingleFlowShopParser {
public:
    SingleFlowShopParser();

    [[nodiscard]] problem::Instance extractInformation(std::string_view fileName,
                                                       const rapidxml::xml_node<> *root,
                                                       cli::ShopType type);

    static void loadMaintenancePolicy(problem::Instance &instance, std::string fname);

private:
    using TimeTable = utils::containers::TwoKeyMap<problem::Operation, delay>;

    std::uint32_t numberOfJobs;

    /// Array of sheet size indicators. Size must equal to total number of operations.
    OperationSizes::Table sheetSizes;

    /// Mapping of a job to an absolute due date
    problem::JobsTime m_absoluteDueDates;

    /// Mapping of a machine to its module
    std::map<problem::MachineId, problem::ModuleId> m_machineToModule;

    /// Listing of jobs and their operations
    problem::JobOperations m_jobOperationsSet;

    /// Mapping of operations to machines
    problem::OperationMachineMap m_operationMachineMap;

    // default and maximum values
    unsigned int m_defaultSheetSize;
    delay m_maximumSheetSize;

    void loadFlowVector(const rapidxml::xml_node<> *root, const rapidxml::xml_node<> *fvNode);

    void loadFlowVectorV1(const rapidxml::xml_node<> *root,
                          const rapidxml::xml_node<> *flowVectorNode);

    [[nodiscard]] static problem::DefaultOperationsTime
    loadProcessingTimes(const rapidxml::xml_node<> *pTimes);

    [[nodiscard]] std::tuple<problem::DefaultTimeBetweenOps, problem::TimeBetweenOps>
    loadSetupTimes(const rapidxml::xml_node<> *root);

    [[nodiscard]] std::tuple<TimeTable, TimeTable> loadDueDates(const rapidxml::xml_node<> *root);

    void loadSheetSizes(const rapidxml::xml_node<> *sSizes);

    [[nodiscard]] static std::map<problem::MaintType, delay>
    loadMaintenanceProcessingTimes(const rapidxml::xml_node<> *pTimes);

    [[nodiscard]] static std::map<problem::MaintType, std::tuple<delay, delay>>
    loadMaintenanceThresholds(const rapidxml::xml_node<> *thresh);

    [[nodiscard]] std::tuple<TimeTable, std::optional<delay>>
    loadTimeTable(const rapidxml::xml_node<> *node, std::string_view subNodes);

    [[nodiscard]] static problem::PlexityTable loadJobPlexity(const rapidxml::xml_node<> *jp);
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

    [[nodiscard]] problem::Instance
    createFlowShop(cli::ShopType type = cli::ShopType::FIXEDORDERSHOP);

    [[nodiscard]] problem::ProductionLine
    createProductionLine(cli::ShopType type = cli::ShopType::FIXEDORDERSHOP);

    [[nodiscard]] inline FileType getFileType() const {
        if (!isLoaded()) {
            throw std::runtime_error("XML file not loaded");
        }
        return m_fileType;
    }

    inline static void loadMaintenancePolicy(problem::Instance &instance, std::string fname) {
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

    [[nodiscard]] static problem::ModulesTransferConstraints
    loadTransferPoints(const rapidxml::xml_node<> *topNode,
                       const std::unordered_map<problem::ModuleId, problem::Instance> &modules);

    [[nodiscard]] static std::tuple<std::unordered_map<problem::JobId, delay>, std::optional<delay>>
    loadJobTimings(const rapidxml::xml_node<> *node,
                   std::string_view subNodes,
                   const problem::JobOperations &jobOperations);
};

} // namespace fms::problem

#endif /* FMS_PROBLEM_XML_PARSER_HPP */
