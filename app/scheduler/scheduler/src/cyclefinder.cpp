#include "cyclefinder.h"

bool CycleFinder::has_for_cycle_to(unsigned int node_index,
                                   std::vector<DelayGraph::VertexID> edgeTo) {
    std::vector<delay> visited(edgeTo.size());

    unsigned int count = 0;
    while(node_index != 0 && count < edgeTo.size()) { // either at start, or not set yet
        if(visited[node_index]) {
            // cycle detected

            extract_cycle_to(visited[node_index], edgeTo);
            return true;
        }
        if(marked[node_index]) {
            // this did not lead to a cycle before, we can stop
            break;
        }
        marked[node_index] = true;
        visited[node_index] = true;
        node_index = edgeTo[node_index];

        count++;
    }
    return false;
}

void CycleFinder::extract_cycle_to(const unsigned int node_index,
                                   std::vector<DelayGraph::VertexID> edgeTo) {
    unsigned int n_index = node_index;
    unsigned int count = 0;
    while(count < edgeTo.size()) {
        cycle.push_back(n_index);
        n_index = edgeTo.at(n_index);
        count++;
        if(node_index == n_index) {
            cycle.push_back(n_index);
            break;
        }
    }
}

CycleFinder::CycleFinder(std::vector<DelayGraph::VertexID> edgeTo) {
    marked.resize(edgeTo.size());
    for(unsigned int i=0; i < edgeTo.size(); i++) {
        if(edgeTo[i] >= edgeTo.size()) {
            throw std::runtime_error("Vertex index is larger than the number of vertices provided");
        }
    }

    for(unsigned int i=0; i < edgeTo.size(); i++) {
        if(has_for_cycle_to(i, edgeTo)) {
            break;
        }
    }
}
