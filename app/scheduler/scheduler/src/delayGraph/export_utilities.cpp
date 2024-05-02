#include "delayGraph/export_utilities.h"

#include "FORPFSSPSD/operation.h"
#include "delay.h"
#include "delayGraph/delayGraph.h"
#include "delayGraph/exportStrings.hpp"

#include "pch/containers.hpp"

#include <algorithm>
#include <fmt/compile.h>
#include <fmt/ostream.h>
#include <fstream>
#include <ios>
#include <rapidxml.hpp>
#include <rapidxml_print.hpp>

using namespace DelayGraph;

namespace {

rapidxml::xml_node<> *createXMLEdge(rapidxml::xml_document<> &doc, const edge &e) {
    rapidxml::xml_node<> *edge = doc.allocate_node(rapidxml::node_element, "edge");
    edge->append_attribute(
            doc.allocate_attribute("source", doc.allocate_string(fmt::to_string(e.src).c_str())));
    edge->append_attribute(
            doc.allocate_attribute("target", doc.allocate_string(fmt::to_string(e.dst).c_str())));

    auto *data = doc.allocate_node(rapidxml::node_element, "data");
    data->append_attribute(doc.allocate_attribute("key", "d0"));
    data->value(doc.allocate_string(fmt::to_string(e.weight).c_str()));
    edge->append_node(data);
    return edge;
}

template <bool skipAdded = false>
void addDotEdges(const DelayGraph::delayGraph &dg,
                 const DelayGraph::Edges &edges,
                 std::ostream &file,
                 std::map<VertexID, std::set<VertexID>> &added,
                 const std::string &color = "") {
    constexpr auto edgeFmtSpec = FMT_COMPILE("{0} -> {1} [label=\"{2}\", weight={2}]\n");
    constexpr auto edgeFmtSpecCol =
            FMT_COMPILE("{0} -> {1} [label=\"{2}\", weight={2}, color={3}]\n");

    for (const auto &e : edges) {
        if (dg.is_source(e.src) || dg.is_source(e.dst) || dg.is_terminus(e.dst) ) {
            continue;
        }

        if constexpr (skipAdded) {
            if (added.contains(e.src) && added.at(e.src).contains(e.dst)) {
                continue;
            }
        }

        added[e.src].insert(e.dst);
        if (color.empty()) {
            file << fmt::format(edgeFmtSpec, e.src, e.dst, e.weight);
        } else {
            file << fmt::format(edgeFmtSpecCol, e.src, e.dst, e.weight, color);
        }
    }
}

} // namespace

namespace DelayGraph::export_utilities {

void saveAsTikz(const FORPFSSPSD::Instance &flowshop,
                const delayGraph &dg,
                const std::string &filename,
                const DelayGraph::Edges &highlighted) {
    std::ofstream file(filename);
    if(!file) {
        throw FmsSchedulerException(
                fmt::format("cannot open file name {} to export a graph to Tikz", filename));
    }

    const auto &jobs = flowshop.getJobsOutput();
    FORPFSSPSD::JobId jobStart = *std::min_element(jobs.begin(), jobs.end());
    FORPFSSPSD::JobId jobEnd = *std::max_element(jobs.begin(), jobs.end());

    file << tikz::kPreamble;

    // Generate list of jobs
    std::vector<std::string> jobNames;
    jobNames.reserve(jobs.size());
    for (const auto &jobId : jobs) {
        jobNames.push_back(fmt::format("J{}", jobId + 1));
    }
    fmt::print(file, "\\def\\jobs{{{}}}\n", fmt::join(jobNames, ", "));

    // Generate list of operations
    std::vector<std::string> operationNames;
    for (const auto &operationId : flowshop.getOperationsFlowVector()) {
        operationNames.push_back(fmt::format("O{}", operationId + 1));
    }
    fmt::print(file, "\\def\\operations{{{}}}\n", fmt::join(operationNames, ", "));

    file << tikz::kPrintNodes;

    for (const auto &v : dg.get_vertices()) {
        for (const auto &[dst, weight] : v.get_outgoing_edges()) {
            const auto &srcOp = v.operation;
            const auto &dstOp = dg.get_vertex(dst).operation;

            if (srcOp.jobId < jobStart || srcOp.jobId > jobEnd || dstOp.jobId < jobStart
                || dstOp.jobId > jobEnd) {
                continue; // skip; not inside the window
            }

            std::string style = "setup";
            if (weight < 0) {
                style = "deadline";
            } else if (srcOp.jobId != dstOp.jobId) {
                style = "ssetup";
            }

            fmt::print(file,
                       "\\draw[{}] (J{}O{}) to node[auto]{{\\tiny {}}} (J{}O{});\n",
                       style,
                       srcOp.jobId + 1,
                       srcOp.operationId + 1,
                       weight,
                       dstOp.jobId + 1,
                       dstOp.operationId + 1);
        }
    }

    file << std::endl;
    for (const auto &e : highlighted) {
        const auto &srcOp = dg.get_vertex(e.src).operation;
        const auto &dstOp = dg.get_vertex(e.dst).operation;

        if (srcOp.jobId < jobStart || srcOp.jobId > jobEnd || dstOp.jobId < jobStart
            || dstOp.jobId > jobEnd) {
            continue; // skip; not inside the window
        }

        fmt::print(file,
                   "\\draw[deadline] (J{}O{}) to (J{}O{});\n",
                   srcOp.jobId + 1,
                   srcOp.operationId + 1,
                   dstOp.jobId + 1,
                   dstOp.operationId + 1);
    }

    file << tikz::kEnding;
}

void saveAsTikz(const FORPFSSPSD::Instance &flowshop,
                const PartialSolution &ps,
                const std::string &filename,
                const DelayGraph::Edges &highlighted) {
    delayGraph dg = flowshop.getDelayGraph();
    auto edges = flowshop.infer_pim_edges(ps);
    const auto &choosenEdges = ps.getAllChosenEdges();
    edges.insert(edges.end(), choosenEdges.begin(), choosenEdges.end());

    // insert the edges to the graph
    for (const auto &edge : edges) {
        if (!dg.has_edge(edge.src, edge.dst)) {
            dg.add_edge(edge);
        }
    }

    saveAsTikz(flowshop, dg, filename, highlighted);
}

/* Serializes a graph object as a dot graph
 * */
void saveAsDot(const delayGraph &dg,
               const std::string &filename,
               const std::vector<edge> &solutionEdges,
               const std::vector<edge> &highlighted) {
    std::ofstream file(filename, std::ios_base::out | std::ios_base::trunc);
    if (!file) {
        LOG(LOGGER_LEVEL::FATAL, fmt::format("Cannot open file {} to save graph", filename));
        throw FmsSchedulerException("Conversion to dot failed as cannot open file for writing");
    }

    std::map<VertexID, std::set<VertexID>> added;

    file << "strict digraph G {\n"
            "graph [layout=neato]\n"
            "edge [color=black]\n"
            "node [pin=True]\n";

    // Add first the highlighted edges to make sure that they are highlighted
    addDotEdges(dg, highlighted, file, added, "red");
    addDotEdges(dg, solutionEdges, file, added, "green");

    for (const auto &v : dg.get_vertices()) {
        if (DelayGraph::delayGraph::is_source(v) || DelayGraph::delayGraph::is_terminus(v)) {
            continue;
        }

        const auto &op = v.operation;
        file << fmt::format(
                FMT_COMPILE("{0} [label=\"{0}\\n{1},{2}\", pos=\"{3},{4}!\"];\n"),
                v.id,
                op.jobId,
                op.operationId,
                op.jobId * 2,
                static_cast<int>(op.operationId) * -4);

        DelayGraph::Edges outEdges;
        const auto &outgoingEdges = v.get_outgoing_edges();
        std::transform(outgoingEdges.begin(),
                       outgoingEdges.end(),
                       std::back_inserter(outEdges),
                       [&v](const auto &e) { return edge(v.id, e.first, e.second); });
        addDotEdges(dg, outEdges, file, added);
    }

    file << "}";
    file.close();
}

std::vector<edge> extract_longest_path(const algorithm::LongestPathResult &lpt,
                                       const vertex &destination) {
    if (!lpt.positiveCycle.empty()) {
        throw FmsSchedulerException("Cannot determine value of longest path for a cyclic path!");
    }

    auto current_node = destination.id;
    std::vector<edge> result;

    return result;
}

} // namespace DelayGraph::export_utilities
