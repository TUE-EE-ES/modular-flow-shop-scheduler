#ifndef FORPFSSPSD_EXPORT_UTILITIES_HPP
#define FORPFSSPSD_EXPORT_UTILITIES_HPP

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "FORPFSSPSD/aliases.hpp"
#include "delayGraph/delayGraph.h"

#include <filesystem>
#include <fstream>
#include <optional>
#include <ostream>

namespace FORPFSSPSD::ExportUtilities {

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
               const DelayGraph::delayGraph &delayGraph);

void saveTimeTableXML(std::ofstream &file,
                      const FORPFSSPSD::TimeBetweenOps &table,
                      std::string_view name,
                      std::string_view subNodes,
                      std::optional<delay> defaultDelay = std::nullopt);
} // namespace FORPFSSPSD::ExportUtilities

#endif