#ifndef DOT_PARSER_H
#define DOT_PARSER_H

#include <fms/cg/constraint_graph.hpp>
#include <fms/delay.hpp>
#include <fms/problem/operation.hpp>

#include <QColor>
#include <filesystem>

namespace FlowShopVis::DOT {

using Statements = std::vector<std::string>;
using Option = std::tuple<std::string, std::string>;
using Options = std::vector<Option>;

using ColouredEdges =
        std::unordered_map<fms::cg::VertexId, std::unordered_map<fms::cg::VertexId, QColor>>;

enum class StatementType { NONE, NODE, EDGE };

struct DotFileResult {
    fms::cg::ConstraintGraph graph;
    ColouredEdges colouredEdges;
    std::string firstDigraph;
};

/**
 * @brief Parses the DOT file and returns the graph.
 * @param dotFilePath The path to the DOT file.
 * @return The parsed graph.
 */
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

/**
 * @brief Splits the graph string into statements.
 * @param graphString
 * @return
 */
Statements splitIntoStatements(const std::string &graphString);

/**
 * @brief Obtains the first digraph from the DOT file.
 * @param dotFile
 * @return
 */
std::string getFirstDigraph(const std::string &dotFile);

/**
 * @brief Obtains the edge strings from the statement.
 * @param statement
 * @return
 */
std::tuple<StatementType, std::string, std::string, std::string, std::string>
getEdgeStrings(const std::string &statement);

/**
 * @brief Obtains the key-value pair from the option.
 * @param option
 * @return
 */
Option getOptionsKeyValue(const std::string &option);

/**
 * @brief Obtains the statement options.
 * @param options
 * @return
 */
Options getStatementOptions(const std::string &options);

/**
 * @brief Obtains the operation from the label.
 * @param label
 * @return
 */
std::optional<fms::problem::Operation> getOperationFromLabel(const std::string &label);

/**
 * @brief Obtains the operation from the options.
 * @param options
 * @return
 */
std::optional<fms::problem::Operation> getOperationFromOptions(const Options &options);

/**
 * @brief Obtains the edge weight from the options.
 * @param options
 * @return
 */
std::optional<fms::delay> getEdgeWeightFromOptions(const Options &options);

QColor getColourFromOptions(const Options &options);

} // namespace FlowShopVis::DOT

#endif // DOT_PARSER_H
