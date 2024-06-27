#include "FORPFSSPSD/indices.hpp"
#include <gtest/gtest.h>

#include <algorithm>
#include <delayGraph/delayGraph.h>
#include <negativecyclefinder.h>

using namespace std;
using namespace DelayGraph;

TEST(CYCLE, EmptyGraph) {
    delayGraph dg;

    ASSERT_FALSE(NegativeCycleFinder(dg).hasNegativeCycle());
}

TEST(CYCLE, SingleVertexGraph) {
    delayGraph dg;
    dg.add_source(FORPFSSPSD::MachineId(0u));

    ASSERT_FALSE(NegativeCycleFinder(dg).hasNegativeCycle());
}

TEST(CYCLE, SmallGraphWithNoEdges) {
    delayGraph dg;

    const int numNodes = 5;
    for (std::size_t i = 0; i < numNodes; i++) {
        dg.add_vertex(FS::JobId(i), FS::OperationId(i));
    }

    ASSERT_FALSE(NegativeCycleFinder(dg).hasNegativeCycle());
}

TEST(CYCLE, ManyNegativeCycles) {
    delayGraph dg;

    const int numNodes = 5;
    for (std::size_t i = 0; i < numNodes; i++) {
        dg.add_vertex(FS::JobId(i), FS::OperationId(i));
    }

    for (auto &vertex : dg.get_vertices()) {
        for (auto &vertex2 : dg.get_vertices()) {
            if(vertex == vertex2) {
                continue;
            }
            dg.add_edge(vertex, vertex2, -1);
        }
    }

    ASSERT_TRUE(NegativeCycleFinder(dg).hasNegativeCycle());
}

TEST(CYCLE, ManyPositiveCycles) {
    delayGraph dg;

    const int numNodes = 5;
    for(std::size_t i = 0; i < numNodes; i++) {
        dg.add_vertex(FS::JobId(i), FS::OperationId(i));
    }

    for (auto &vertex : dg.get_vertices()) {
        for (auto &vertex2 : dg.get_vertices()) {
            if(vertex == vertex2) {
                continue;
            }
            dg.add_edge(vertex, vertex2, 1u);
        }
    }

    ASSERT_FALSE(NegativeCycleFinder(dg).hasNegativeCycle());
}

TEST(CYCLE, LongNegativeCycle) {
    delayGraph dg;

    const std::size_t numNodes = 5;
    VertexID prev = -1u;

    for(std::size_t i = 0; i < numNodes; i++) {

        VertexID current = dg.add_vertex(FS::JobId(i), FS::OperationId(i));
        if(prev != -1u) {
            dg.add_edge(prev, current, -1);
        }

        prev = current;
    }
    dg.add_edge(prev, 0, -1);

    ASSERT_TRUE(NegativeCycleFinder(dg).hasNegativeCycle());
}


TEST(CYCLE, TwoVertexNegativeCycle) {
    delayGraph dg;
    auto vId1 = dg.add_vertex(FS::JobId(0U), FS::OperationId(0U));
    auto vId2 = dg.add_vertex(FS::JobId(0U), FS::OperationId(1U));

    auto &v1 = dg.get_vertex(vId1);
    auto &v2 = dg.get_vertex(vId2);
    dg.add_edge(v1, v2, -1);
    dg.add_edge(v2, v1, -1);

    NegativeCycleFinder ncf (dg);
    ASSERT_TRUE(ncf.hasNegativeCycle());
    auto negativeCycle = ncf.getNegativeCycle();
    EXPECT_TRUE(
            std::find_if(negativeCycle.begin(),
                         negativeCycle.end(),
                         [&v1, &v2](const edge &it) { return it.src == v1.id && it.dst == v2.id; })
            != negativeCycle.end());
    EXPECT_TRUE(
            std::find_if(negativeCycle.begin(),
                         negativeCycle.end(),
                         [&v1, &v2](const edge &it) { return it.src == v2.id && it.dst == v1.id; })
            != negativeCycle.end());
}

TEST(CYCLE, TwoVertexPositiveCycle) {
    delayGraph dg;
    auto vId1 = dg.add_vertex(FS::JobId(0U), FS::OperationId(0U));
    auto vId2 = dg.add_vertex(FS::JobId(0U), FS::OperationId(1U));

    auto &v1 = dg.get_vertex(vId1);
    auto &v2 = dg.get_vertex(vId2);
    dg.add_edge(v1, v2, 1);
    dg.add_edge(v2, v1, 1);

    NegativeCycleFinder ncf (dg);
    EXPECT_FALSE(ncf.hasNegativeCycle());
}

TEST(CYCLE, SmallTree) {
    delayGraph dg;

    const int numNodes = 8;
    for(unsigned int i = 0; i < numNodes; i++) {
        dg.add_vertex(FS::JobId(i), FS::OperationId(i));
    }
    auto &vertices = dg.get_vertices();
    EXPECT_TRUE(vertices.at(0).get_incoming_edges().empty());

    dg.add_edge(vertices.at(0), vertices.at(1), 1);
    dg.add_edge(vertices.at(0), vertices.at(2), 2);

    dg.add_edge(vertices.at(1), vertices.at(3), 3);
    dg.add_edge(vertices.at(1), vertices.at(4), 4);

    dg.add_edge(vertices.at(2), vertices.at(5), 5);
    dg.add_edge(vertices.at(2), vertices.at(6), 6);

    EXPECT_FALSE(NegativeCycleFinder(dg).hasNegativeCycle());
}


TEST(CYCLE, InfeasibleExample) {
    delayGraph dg;

    VertexID v1 = dg.add_vertex(FS::JobId(0u), FS::OperationId(0u));
    VertexID v2 = dg.add_vertex(FS::JobId(1u), FS::OperationId(1u));

    dg.add_edge(v1,v2,-1);
    dg.add_edge(v2,v1, 0);

    ASSERT_TRUE(NegativeCycleFinder(dg).hasNegativeCycle());
}

