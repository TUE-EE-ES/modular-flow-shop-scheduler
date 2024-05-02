#include "negativecyclefinder.h"

#include <stack>

NegativeCycleFinder::NegativeCycleFinder(const DelayGraph::delayGraph & graph)
    : graph(graph)
{
    // assume VertexID to range from 0 to |V(G)|-1
    unsigned int nrVertices = graph.get_number_of_vertices();
    onStack.resize(nrVertices);
    edgeTo.resize(nrVertices);
    marked.resize(nrVertices);
    shortestPath.resize(nrVertices);
    for(auto & p : shortestPath) {
        p = std::numeric_limits<delay>::max();
    }

    for(unsigned int v = 0; v < nrVertices; v++) {
        if(!marked[v]) {
            shortestPath[v] = 0;
            findNegativeCycleDFS(v);
        }
    }
}

bool NegativeCycleFinder::hasNegativeCycle()
{
    return !negativeCycle.empty();
}

DelayGraph::Edges NegativeCycleFinder::getNegativeCycle()
{
    return negativeCycle;
}

void NegativeCycleFinder::findNegativeCycleDFS(const DelayGraph::VertexID v) {
    onStack[v] = true;
    marked[v] = true;
    for(const auto& [dst, weight] : graph.get_vertex(v).get_outgoing_edges()) {
        if(hasNegativeCycle()) return; // only interested in finding one negative cycle

        DelayGraph::VertexID w = dst;
        DelayGraph::edge edge{v, w, weight};
        if( !marked[w] ) {
            edgeTo[w] = edge;
            shortestPath[w] = shortestPath[v] + weight;
            findNegativeCycleDFS(w);
        } else if( onStack[w] ) {

            // found a cycle, with negative cost?
            if(shortestPath[w] > shortestPath[v] + weight) {
                std::stack<DelayGraph::edge> stack;

                // following the cycle leads to lower value than before following
                // the cycle; this must be a negative cycle.

                for (auto val = edgeTo[v];
                     val.has_value() && val.value().dst != w;
                     val = edgeTo[val.value().src]) {
                    stack.push(val.value());
                }

                stack.push(edge);
                while(!stack.empty()) {
                    negativeCycle.push_back(stack.top());
                    stack.pop();
                }
                return;
            }
        }
    }
    onStack[v] = false;
}
