#ifndef FMS_PROBLEM_EXPORT_UTILITIES_HPP
#define FMS_PROBLEM_EXPORT_UTILITIES_HPP

#include "aliases.hpp"
#include "flow_shop.hpp"

#include "fms/cg/constraint_graph.hpp"

#include <filesystem>
#include <fstream>
#include <optional>

namespace fms::problem::ExportUtilities {

namespace fs = std::filesystem;

/**
 * @brief Save a FORPFSSPSD instance as an XML file.
 *
 * @param outputPath Path of the output XML file.
 * @param instance Instance to save.
 */
void saveAsXML(const fs::path &outputPath, const Instance &instance);

/**
 * @brief Save a FORPFSSPSD instance as an XML file and include the maintenance operations of the
 * delay graph if they are present.
 *
 * @param outputPath Path of the output XML file.
 * @param instance Instance to save.
 * @param delayGraph Delay graph with extra maintenance operations.
 */
void saveAsXML(const fs::path &outputPath,
               const Instance &instance,
               const cg::ConstraintGraph &delayGraph);

void saveTimeTableXML(std::ofstream &file,
                      const problem::TimeBetweenOps &table,
                      std::string_view name,
                      std::string_view subNodes,
                      std::optional<delay> defaultDelay = std::nullopt);
} // namespace fms::problem::ExportUtilities

#endif // FMS_PROBLEM_EXPORT_UTILITIES_HPP