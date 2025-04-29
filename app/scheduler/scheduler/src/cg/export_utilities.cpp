#include "fms/pch/containers.hpp"
#include "fms/pch/fmt.hpp"

#include "fms/cg/export_utilities.hpp"

#include "fms/cg/constraint_graph.hpp"
#include "fms/cg/export_strings.hpp"
#include "fms/problem/operation.hpp"

#include <algorithm>
#include <fstream>
#include <ios>
#include <rapidxml/rapidxml.hpp>
#include <rapidxml/rapidxml_print.hpp>

using namespace fms;
using namespace fms::cg;
using namespace fms::cg::exports;

namespace {

rapidxml::xml_node<> *createXMLEdge(rapidxml::xml_document<> &doc, const Edge &e) {
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
void addDotEdges(const ConstraintGraph &dg,
                 const Edges &edges,
                 std::ostream &file,
                 std::map<VertexId, std::set<VertexId>> &added,
                 const std::string &color = "") {
    constexpr auto edgeFmtSpec = FMT_COMPILE("{0} -> {1} [label=\"{2}\", weight={2}]\n");
    constexpr auto edgeFmtSpecCol =
            FMT_COMPILE("{0} -> {1} [label=\"{2}\", weight={2}, color={3}]\n");

    for (const auto &e : edges) {
        if (dg.isSource(e.src) || dg.isSource(e.dst) || dg.isTerminus(e.dst)) {
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

void exports::saveAsTikz(const problem::Instance &flowshop,
                         const ConstraintGraph &dg,
                         const std::string &filename,
                         const Edges &highlighted) {
    std::ofstream file(filename);
    if (!file) {
        throw FmsSchedulerException(
                fmt::format("cannot open file name {} to export a graph to Tikz", filename));
    }

    const auto &jobs = flowshop.getJobsOutput();
    problem::JobId jobStart = *std::min_element(jobs.begin(), jobs.end());
    problem::JobId jobEnd = *std::max_element(jobs.begin(), jobs.end());

    file << strings::tikz::kPreamble;

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

    file << strings::tikz::kPrintNodes;

    for (const auto &v : dg.getVertices()) {
        for (const auto &[dst, weight] : v.getOutgoingEdges()) {
            const auto &srcOp = v.operation;
            const auto &dstOp = dg.getVertex(dst).operation;

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
        const auto &srcOp = dg.getVertex(e.src).operation;
        const auto &dstOp = dg.getVertex(e.dst).operation;

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

    file << strings::tikz::kEnding;
}

void exports::saveAsTikz(const problem::Instance &flowshop,
                         const solvers::PartialSolution &ps,
                         const std::string &filename,
                         const Edges &highlighted) {
    ConstraintGraph dg = flowshop.getDelayGraph();
    auto edges = ps.getAllAndInferredEdges(flowshop);

    // insert the edges to the graph
    for (const auto &edge : edges) {
        if (!dg.hasEdge(edge.src, edge.dst)) {
            dg.addEdges(edge);
        }
    }

    saveAsTikz(flowshop, dg, filename, highlighted);
}

/* Serializes a graph object as a dot graph
 * */
void exports::saveAsDot(const ConstraintGraph &dg,
                        const std::string &filename,
                        const Edges &solutionEdges,
                        const Edges &highlighted) {
    std::ofstream file(filename, std::ios_base::out | std::ios_base::trunc);
    if (!file) {
        LOG_C("Cannot open file {} to save graph", filename);
        throw FmsSchedulerException("Conversion to dot failed as cannot open file for writing");
    }

    std::map<VertexId, std::set<VertexId>> added;

    file << "strict digraph G {\n"
            "graph [layout=neato]\n"
            "edge [color=black]\n"
            "node [pin=True]\n";

    // Add first the highlighted edges to make sure that they are highlighted
    addDotEdges(dg, highlighted, file, added, "red");
    addDotEdges<true>(dg, solutionEdges, file, added, "green");

    for (const auto &v : dg.getVertices()) {
        if (ConstraintGraph::isSource(v) || ConstraintGraph::isTerminus(v)) {
            continue;
        }

        const auto &op = v.operation;
        file << fmt::format(FMT_COMPILE("{0} [label=\"{0}\\n{1},{2}\", pos=\"{3},{4}!\"];\n"),
                            v.id,
                            op.jobId,
                            op.operationId,
                            op.jobId.value * 2,
                            static_cast<int>(op.operationId) * -4);

        Edges outEdges;
        const auto &outgoingEdges = v.getOutgoingEdges();
        std::transform(outgoingEdges.begin(),
                       outgoingEdges.end(),
                       std::back_inserter(outEdges),
                       [&v](const auto &e) { return Edge(v.id, e.first, e.second); });
        addDotEdges(dg, outEdges, file, added);
    }

    file << "}";
    file.close();
}

Edges exports::extract_longest_path(const algorithms::paths::LongestPathResult &lpt,
                                    const Vertex &destination) {
    if (!lpt.positiveCycle.empty()) {
        throw FmsSchedulerException("Cannot determine value of longest path for a cyclic path!");
    }

    auto current_node = destination.id;
    Edges result;

    return result;
}
