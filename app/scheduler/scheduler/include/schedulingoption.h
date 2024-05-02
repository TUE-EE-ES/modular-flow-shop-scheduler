#ifndef SCHEDULING_OPTION_H
#define SCHEDULING_OPTION_H

#include <utility>

#include "delayGraph/delayGraph.h"

namespace algorithm {
/**
 * Scheduling Option structure for the scheduling heuristics
 * Serves as option to add an operation somewhere in a schedule 
 * Or to remove an operation form somewhere in a schedule
 */
struct option {

    option(DelayGraph::edge prevE,
           DelayGraph::edge nextE,
           DelayGraph::VertexID prevV,
           DelayGraph::VertexID curV,
           DelayGraph::VertexID nextV,
           unsigned int position,
           bool is_maint = false) :
        prevE(prevE),
        nextE(nextE),
        prevV(prevV),
        curV(curV),
        nextV(nextV),
        position(position),
        is_maint(is_maint) {}

    // assignment operator
    option& operator=(const option& other) {
        if (&other == this) {
            return *this;
        }
        prevE = other.prevE;
        nextE = other.nextE;
        weight = other.weight;
        prevV = other.prevV;
        curV = other.curV;
        nextV = other.nextV;
        position = other.position;
        return *this;
    }

    std::uint64_t id;
    long double weight;

    // assuming that prevV->curV  was in the chosenEdges previously, x->y is preE, and
    DelayGraph::edge prevE, nextE;

    DelayGraph::VertexID
            prevV, // prevV is the operation directly before curV in the sequence chosen so far
            curV,  // curV is the operation currently being sequenced in between prevV and nextV
            nextV; // nextV is the operation directly after curV in the sequence chosen so far

    // it's assumed that in the current partial solution, the chosenEdges field contains the edge
    // prevV->nextV at the index denoted by insertion_point
    unsigned int position;
    //holds true if currV is a maintenance vertex
    bool is_maint;

};
} // namespace algorithm
#endif // SCHEDULING_OPTION_H
