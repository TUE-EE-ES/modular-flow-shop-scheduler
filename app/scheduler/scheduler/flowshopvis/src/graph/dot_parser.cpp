#include "dot_parser.hpp"

#include <fms/utils/strings.hpp>

#include <QMessageBox>
#include <fstream>
#include <regex>

namespace FlowShopVis::DOT {

void sanitizeAndAddStatement(const std::size_t start,
                             const std::size_t end,
                             const std::string &graphString,
                             std::vector<std::string> &statements) {
    auto string = graphString.substr(start, end - start);
    fms::utils::strings::trim(string);

    // We do this to handle ';\n' and ';\n\n' cases
    if (!string.empty() && string != " ") {
        statements.push_back(string);
    }
}

Statements splitIntoStatements(const std::string &graphString) {
    // Split by statements. They are separated by a semicolon or by a new line. Handle nested
    // braces, escaping quotes and comments.
    std::vector<std::string> statements;
    std::size_t start = 0;
    std::size_t end = 0;
    std::int32_t braceCount = 0;
    bool inQuotes = false;
    bool inComment = false;
    while (end < graphString.size()) {
        if (graphString[end] == '{') {
            braceCount++;
        } else if (graphString[end] == '}') {
            braceCount--;
        } else if (graphString[end] == '"' && end > 0 && graphString[end - 1] != '\\') {
            inQuotes = !inQuotes;
        } else if (graphString[end] == '/' && end + 1 < graphString.size()
                   && graphString[end + 1] == '/') {
            inComment = true;
        } else if (graphString[end] == '\n') {
            inComment = false;
        }

        if ((graphString[end] == ';' || graphString[end] == '\n') && braceCount == 0 && !inQuotes
            && !inComment) {
            sanitizeAndAddStatement(start, end, graphString, statements);
            start = end + 1;
        }

        end++;
    }
    // Add whathever was remaining
    sanitizeAndAddStatement(start, end, graphString, statements);
    return statements;
}

std::string getFirstDigraph(const std::string &dotFile) {
    // Find the first digraph
    std::size_t pos = dotFile.find("digraph");

    // Find the first opening brace
    pos = dotFile.find('{', pos);

    // Find the first closing brace but handling nested braces
    std::size_t closingBrace = pos;
    int braceCount = 1;

    while (braceCount > 0) {
        closingBrace++;
        if (dotFile[closingBrace] == '{') {
            braceCount++;
        } else if (dotFile[closingBrace] == '}') {
            braceCount--;
        }
    }

    auto graphString = dotFile.substr(pos + 1, closingBrace - pos - 1);
    fms::utils::strings::trim(graphString);
    return graphString;
}
std::tuple<StatementType, std::string, std::string, std::string, std::string>
getEdgeStrings(const std::string &statement) {
    // Use regular expression for matching nodes and edges
    std::regex nodeRegex(R"[[(([\w\->\s]+)(\[.*\])?)[[");
    std::smatch match;

    if (!std::regex_search(statement, match, nodeRegex)) {
        return {StatementType::NONE, "", "", "", ""};
    }

    std::string node = match[1];
    fms::utils::strings::trim(node);

    // Skip if node is called "graph", "edge" or "node" as these are reserved words, or if there are
    // no options on the edge/node
    if (node == "graph" || node == "edge" || node == "node" || !match[2].matched) {
        return {StatementType::NONE, "", "", "", ""};
    }

    // Skip first and last characters as they are '[' and ']'
    std::string options = match[2];
    fms::utils::strings::trim(options);
    options = options.substr(1, options.size() - 2);

    if (node.find("->") == std::string::npos) {
        // It's a node
        return {StatementType::NODE, std::move(node), "", options, ""};
    }

    std::regex edgeRegex(R"[[(([\w]+)\s*->\s*([\w]+))[[");
    std::smatch edgeMatch;

    if (!std::regex_search(node, edgeMatch, edgeRegex)) {
        return {StatementType::NONE, "", "", "", fmt::format("Error parsing edge: {}", node)};
    }

    std::string src = edgeMatch[1];
    std::string dst = edgeMatch[2];
    fms::utils::strings::trim(src);
    fms::utils::strings::trim(dst);

    return {StatementType::EDGE, std::move(src), std::move(dst), std::move(options), ""};
}
Option getOptionsKeyValue(const std::string &option) {
    // Options are divided into key=value pairs, find the first '=' and split the string
    std::size_t pos = option.find('=');

    // It is possible to have keys without value
    if (pos == std::string::npos) {
        return {option, ""};
    }

    std::string key = option.substr(0, pos);
    std::string value = option.substr(pos + 1);

    fms::utils::strings::trim(key);
    fms::utils::strings::trim(value);

    // Remove quotes from value if any
    if (value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.size() - 2);
    }

    return {key, value};
}
Options getStatementOptions(const std::string &options) {
    // Divide options into statements, separated by a comma and ignoring commas inside quotes
    Options optionStatements;
    std::size_t start = 0;
    std::size_t end = 0;
    bool inQuotes = false;
    while (end < options.size()) {
        if (options[end] == '"' && end > 0 && options[end - 1] != '\\') {
            inQuotes = !inQuotes;
        }

        if (options[end] == ',' && !inQuotes) {
            auto string = options.substr(start, end - start);
            fms::utils::strings::trim(string);
            if (!string.empty()) {
                optionStatements.push_back(getOptionsKeyValue(string));
            }
            start = end + 1;
        }

        end++;
    }

    // Add whathever was remaining
    auto string = options.substr(start, end - start);
    fms::utils::strings::trim(string);
    if (!string.empty()) {
        optionStatements.push_back(getOptionsKeyValue(string));
    }
    return optionStatements;
}
std::optional<fms::problem::Operation> getOperationFromLabel(const std::string &label) {
    std::regex operationRegex(R"[[(\D*(\d+)\s*,\s*(\d+)\s*$)[[");
    std::smatch match;

    if (!std::regex_search(label, match, operationRegex)) {
        return std::nullopt;
    }

    try {
        // NOLINTNEXTLINE(google-readability-casting): Wrongly flagged redundant cast
        return fms::problem::Operation(fms::problem::JobId(std::stoi(match[1])),
                                       static_cast<fms::problem::OperationId>(std::stoi(match[2])));
    } catch (const std::invalid_argument &e) {
        return std::nullopt;
    }
}
std::optional<fms::problem::Operation> getOperationFromOptions(const Options &options) {
    for (const auto &[key, value] : options) {
        if (key == "label") {
            return getOperationFromLabel(value);
        }
    }
    return std::nullopt;
}
std::optional<fms::delay> getEdgeWeightFromOptions(const Options &options) {
    for (const auto &[key, value] : options) {
        if (key == "weight") {
            try {
                return std::stoi(value);
            } catch (const std::invalid_argument &e) {
                QMessageBox::critical(
                        nullptr, "Error", fmt::format("Error parsing weight: {}", value).c_str());
                return std::nullopt;
            }
        }
    }
    return std::nullopt;
}
QColor getColourFromOptions(const Options &options) {
    for (const auto &[key, value] : options) {
        if (key == "color") {
            return {QString::fromStdString(value)};
        }
    }
    return {};
}

DotFileResult parseDotFile(const std::filesystem::path &dotFilePath) {
    std::ifstream file(dotFilePath);
    std::string dotFile((std::istreambuf_iterator<char>(file)), {});

    std::string graphString = getFirstDigraph(dotFile);
    auto statements = splitIntoStatements(graphString);

    // Create the graph
    DotFileResult result;
    std::unordered_map<std::string, fms::cg::VertexId> vertexMap;
    std::vector<std::tuple<std::string, std::string, fms::delay, QColor>> edges;

    // Add vertices and edges
    for (const auto &statement : statements) {
        // Use regular expression for matching nodes and edges
        auto [type, src, dst, optionsStr, error] = getEdgeStrings(statement);

        if (!error.empty()) {
            return {{}, {}, std::move(error)};
        }

        if (type == StatementType::NONE) {
            continue;
        }

        const auto options = getStatementOptions(optionsStr);

        if (type == StatementType::NODE) {
            const auto operation = getOperationFromOptions(options);
            if (!operation.has_value()) {
                continue;
            }

            if (vertexMap.find(src) == vertexMap.end()) {
                vertexMap[src] = result.graph.addVertex(operation.value());
            }
        } else {
            // It's an edge, find the weight option
            std::optional<fms::delay> weight = getEdgeWeightFromOptions(options);
            if (!weight.has_value()) {
                return {};
            }

            auto color = getColourFromOptions(options);
            edges.emplace_back(src, dst, *weight, color);
        }
    }

    // Add all the edges in the graph
    for (const auto &[src, dst, weight, color] : edges) {
        // Check that the vertices exist
        const auto itSrc = vertexMap.find(src);
        if (itSrc == vertexMap.end()) {
            return {{}, {}, fmt::format("Vertex {} does not exist", src)};
        }

        const auto itDst = vertexMap.find(dst);
        if (itDst == vertexMap.end()) {
            return {{}, {}, fmt::format("Vertex {} does not exist", dst)};
        }

        const auto vSrc = itSrc->second;
        const auto vDst = itDst->second;

        result.graph.addEdge(vSrc, vDst, weight);

        if (color.isValid()) {
            result.colouredEdges[vSrc][vDst] = color;
        }
    }

    return result;
}

} // namespace FlowShopVis::DOT
