#ifndef DELAY_GRAPH_EXPORT_UTILITIES_H
#define DELAY_GRAPH_EXPORT_UTILITIES_H

#include "FORPFSSPSD/FORPFSSPSD.h"
#include "delayGraph/delayGraph.h"
#include "longest_path.h"
#include "partialsolution.h"

#include <string>
#include <vector>

namespace DelayGraph::export_utilities {
std::vector<edge> extract_longest_path(const algorithm::LongestPathResult &lpt,
                                       const vertex &destination);

void saveAsTikz(const FORPFSSPSD::Instance &flowshop,
                const delayGraph &dg,
                const std::string &filename,
                const std::vector<edge> &highlighted);

inline void saveAsTikz(const FORPFSSPSD::Instance &flowshop,
                       const delayGraph &dg,
                       const std::string &filename) {
    saveAsTikz(flowshop, dg, filename, {});
}

void saveAsTikz(const FORPFSSPSD::Instance &flowshop,
                const PartialSolution &ps,
                const std::string &filename,
                const DelayGraph::Edges &highlighted);

inline void saveAsTikz(const FORPFSSPSD::Instance &flowshop,
                       const PartialSolution &ps,
                       const std::string &filename) {
    saveAsTikz(flowshop, ps, filename, {});
}

void saveAsDot(const delayGraph &dg,
               const std::string &filename,
               const std::vector<edge> &solutionEdges = {},
               const std::vector<edge> &highlighted = {});
inline void saveAsDot(const FORPFSSPSD::Instance &flowshop,
                      const PartialSolution &ps,
                      const std::string &filename,
                      const std::vector<edge> &highlighted = {}) {
    saveAsDot(flowshop.getDelayGraph(), filename, ps.getAllChosenEdges(), highlighted);
}
} // namespace DelayGraph::export_utilities

#endif // DELAY_GRAPH_EXPORT_UTILITIES_H
