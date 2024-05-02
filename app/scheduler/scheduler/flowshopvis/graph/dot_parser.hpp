#ifndef DOT_PARSER_H
#define DOT_PARSER_H

#include <FORPFSSPSD/operation.h>
#include <delay.h>
#include <delayGraph/delayGraph.h>

#include <QColor>
#include <filesystem>

namespace FlowShopVis::DOT {

using Statements = std::vector<std::string>;
using Option = std::tuple<std::string, std::string>;
using Options = std::vector<Option>;

using ColouredEdges =
        std::unordered_map<DelayGraph::VertexID, std::unordered_map<DelayGraph::VertexID, QColor>>;

enum class StatementType { NONE, NODE, EDGE };

struct DotFileResult {
    DelayGraph::delayGraph graph;
    ColouredEdges colouredEdges;
    std::string firstDigraph;
};

DotFileResult parseDotFile(const std::filesystem::path &dotFilePath);

/**
 * @brief Obtains the substring of graphString, sanitizes it and adds it to the statements vector.
 * @details The substring is sanitized by removing leading and trailing whitespaces and newlines.
 * @param start The starting index of the substring in graphString.
 * @param end The ending index of the substring in graphString.
 * @param graphString The input graph string.
 * @param statements The vector to store the sanitized statements.
 */
void sanitizeAndAddStatement(std::size_t start,
                             std::size_t end,
                             const std::string &graphString,
                             std::vector<std::string> &statements);

Statements splitIntoStatements(const std::string &graphString);

std::string getFirstDigraph(const std::string &dotFile);

std::tuple<StatementType, std::string, std::string, std::string, std::string>
getEdgeStrings(const std::string &statement);

Option getOptionsKeyValue(const std::string &option);

Options getStatementOptions(const std::string &options);

std::optional<FORPFSSPSD::operation> getOperationFromLabel(const std::string &label);

std::optional<FORPFSSPSD::operation> getOperationFromOptions(const Options &options);

std::optional<delay> getEdgeWeightFromOptions(const Options &options);

QColor getColourFromOptions(const Options &options);

} // namespace FlowShopVis::DOT

#endif // DOT_PARSER_H
