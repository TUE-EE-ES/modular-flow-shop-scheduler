#include "pch/containers.hpp"

#include "FORPFSSPSD/export_utilities.hpp"

#include "FORPFSSPSD/aliases.hpp"
#include "FORPFSSPSD/operation.h"
#include "delayGraph/vertex.h"

#include <fmt/ostream.h>
#include <fstream>

using namespace FORPFSSPSD;
using namespace FORPFSSPSD::ExportUtilities;

void FORPFSSPSD::ExportUtilities::saveAsXML(const fs::path &outputPath, const Instance &instance) {
    std::ofstream file(outputPath);
    fmt::print(file, "<SPInstance type=\"FORPFSSPSD\">\n");
    fmt::print(file, "\t<jobs count=\"{}\"/>\n", instance.jobs().size());

    fmt::print(file, "\t<flowVector>\n");
    for (const auto &[jobId, jobOps] : instance.jobs()) {
        for (const auto &op : jobOps) {
            const auto machineId = instance.machineMapping().at(op);
            fmt::print(file,
                       "\t\t<component index=\"{}\" value=\"{}\" job=\"{}\"/>\n",
                       op.operationId,
                       machineId,
                       jobId);
        }
    }
    fmt::print(file, "\t</flowVector>\n");

    fmt::print(file, "\t<processingTimes default=\"0\">\n");
    for (const auto &[op, value] : instance.processingTimes()) {
        fmt::print(file,
                   "\t\t<p j=\"{}\" op=\"{}\" value=\"{}\"/>\n",
                   op.jobId,
                   op.operationId,
                   value);
    }
    fmt::print(file, "\t</processingTimes>\n");

    fmt::print(file, "\t<sizes default=\"0\">\n");
    for (const auto &[op, size] : instance.sheetSizes()) {
        fmt::print(
                file, "\t\t<z j=\"{}\" op=\"{}\" value=\"{}\"/>\n", op.jobId, op.operationId, size);
    }
    fmt::print(file, "\t</sizes>\n");

    saveTimeTableXML(file, instance.setupTimes().table(), "setupTimes", "s");
    saveTimeTableXML(file, instance.setupTimesIndep().table(), "setupTimesIndep", "s");
    saveTimeTableXML(file, instance.dueDates(), "relativeDueDates", "d");
    saveTimeTableXML(file, instance.dueDatesIndep(), "relativeDueDatesIndep", "d");

    fmt::print(file, "\t<jobPlexity>\n");
    for (const auto &[jobId, allReEntrancies] : instance.getPlexityTable()) {
        for (std::size_t i = 0; i < allReEntrancies.size(); ++i) {
            fmt::print(file,
                       "\t\t<t j=\"{}\" Type=\"{}\" id=\"{}\"/>\n",
                       jobId,
                       allReEntrancies[i],
                       i);
        }
    }
    fmt::print(file, "\t</jobPlexity>\n");
}

void FORPFSSPSD::ExportUtilities::saveAsXML(const fs::path &outputPath,
               const Instance &instance,
               const DelayGraph::delayGraph &delayGraph) {
    auto jobs = instance.jobs();
    auto processingTimes = instance.processingTimes();
    auto machineMapping = instance.machineMapping();

    JobId jobId = FS::JobId(jobs.size());
    
    for (const DelayGraph::vertex &v : delayGraph.get_maint_vertices()) {
        OperationsVector ops{{jobId, 0}, {jobId, 1}, {jobId, 2}, {jobId, 3}};
        jobs[jobId] = ops;

        const auto val = instance.maintenancePolicy().getMaintDuration(v.operation.maintId);
        std::vector<delay> values{0, val, 0, 0};
        for (std::size_t i = 0; i < ops.size(); ++i) {
            processingTimes.insert(ops[i], values[i]);
            machineMapping.insert({ops[i],MachineId(0)});
        }
    }

    Instance newInstance{instance.getProblemName(),
                         jobs,
                         machineMapping,
                         processingTimes,
                         instance.setupTimes(),
                         instance.setupTimesIndep(),
                         instance.dueDates(),
                         instance.dueDatesIndep(),
                         instance.absoluteDueDates(),
                         instance.sheetSizes(),
                         instance.defaultSheetSize(),
                         instance.maximumSheetSize(),
                         instance.shopType(),
                         instance.isOutOfOrder()};
    saveAsXML(outputPath, newInstance);
}

void FORPFSSPSD::ExportUtilities::saveTimeTableXML(std::ofstream &file,
                      const FORPFSSPSD::TimeBetweenOps &table,
                      std::string_view name,
                      std::string_view subNodes,
                      std::optional<delay> defaultDelay) {
    if (defaultDelay.has_value()) {
        fmt::print(file, "\t<{} default=\"{}\">\n", name, defaultDelay.value());
    } else {
        fmt::print(file, "\t<{}>\n", name);
    }

    for (const auto &[op1, dest] : table) {
        for (const auto &[op2, value] : dest) {
            fmt::print(file,
                       "\t\t<{} j1=\"{}\" op1=\"{}\" j2=\"{}\" op2=\"{}\" value=\"{}\"/>\n",
                       subNodes,
                       op1.jobId,
                       op1.operationId,
                       op2.jobId,
                       op2.operationId,
                       value);
        }
    }
    fmt::print(file, "\t</{}>\n", name);
}