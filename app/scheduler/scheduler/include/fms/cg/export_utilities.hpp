#ifndef FMS_CG_EXPORT_UTILITIES_HPP
#define FMS_CG_EXPORT_UTILITIES_HPP

#include "constraint_graph.hpp"

#include "fms/problem/flow_shop.hpp"
#include "fms/algorithms/longest_path.hpp"
#include "fms/solvers/partial_solution.hpp"

#include <string>
#include <vector>

/**
 * @brief Namespace for utility functions related to exporting the delay graph
 */
namespace fms::cg::exports {

/**
 * @brief Extracts the longest path from the given result to the destination vertex
 * @param lpt The longest path result
 * @param destination The destination vertex
 * @return A vector of edges representing the longest path
 */
std::vector<Edge> extract_longest_path(const fms::algorithms::paths::LongestPathResult &lpt,
                                       const Vertex &destination);

/**
 * @brief Saves the delay graph as a TikZ file
 * @param flowshop The flowshop instance
 * @param dg The delay graph
 * @param filename The name of the file to save
 * @param highlighted A vector of edges to highlight in the output
 */
void saveAsTikz(const problem::Instance &flowshop,
                const ConstraintGraph &dg,
                const std::string &filename,
                const Edges &highlighted);

/**
 * @brief Saves the delay graph as a TikZ file with no highlighted edges
 * @param flowshop The flowshop instance
 * @param dg The delay graph
 * @param filename The name of the file to save
 */
inline void saveAsTikz(const problem::Instance &flowshop,
                       const ConstraintGraph &dg,
                       const std::string &filename) {
    saveAsTikz(flowshop, dg, filename, {});
}

/**
 * @brief Saves the partial solution as a TikZ file
 * @param flowshop The flowshop instance
 * @param ps The partial solution
 * @param filename The name of the file to save
 * @param highlighted A vector of edges to highlight in the output
 */
void saveAsTikz(const problem::Instance &flowshop,
                const solvers::PartialSolution &ps,
                const std::string &filename,
                const Edges &highlighted);

/**
 * @brief Saves the partial solution as a TikZ file with no highlighted edges
 * @param flowshop The flowshop instance
 * @param ps The partial solution
 * @param filename The name of the file to save
 */
inline void saveAsTikz(const problem::Instance &flowshop,
                       const solvers::PartialSolution &ps,
                       const std::string &filename) {
    saveAsTikz(flowshop, ps, filename, {});
}

/**
 * @brief Saves the delay graph as a DOT file
 * @param dg The delay graph
 * @param filename The name of the file to save
 * @param solutionEdges A vector of edges representing the solution
 * @param highlighted A vector of edges to highlight in the output
 */
void saveAsDot(const ConstraintGraph &dg,
               const std::string &filename,
               const Edges &solutionEdges = {},
               const Edges &highlighted = {});

/**
 * @brief Saves the partial solution as a DOT file
 * @param flowshop The flowshop instance
 * @param ps The partial solution
 * @param filename The name of the file to save
 * @param highlighted A vector of edges to highlight in the output
 */
inline void saveAsDot(const problem::Instance &flowshop,
                      const solvers::PartialSolution &ps,
                      const std::string &filename,
                      const std::vector<Edge> &highlighted = {}) {
    saveAsDot(flowshop.getDelayGraph(), filename, ps.getAllChosenEdges(flowshop), highlighted);
}
} // namespace fms::cg::exports

#endif // FMS_CG_EXPORT_UTILITIES_HPP
