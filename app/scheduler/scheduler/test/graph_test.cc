#include <gtest/gtest.h>

#include <array>
#include <delayGraph/delayGraph.h>


using namespace std;
using namespace DelayGraph;

// NOLINTBEGIN(*-magic-numbers)

TEST(GRAPH, EmptyGraph) {
    delayGraph dg;

    EXPECT_EQ(dg.get_number_of_vertices(), 0U);
    EXPECT_TRUE(dg.get_vertices().empty());
}

TEST(GRAPH, SingleVertexGraph) {
    delayGraph dg;
    VertexID v_id = dg.add_vertex(FS::JobId(0U), 0U);
    EXPECT_EQ(dg.get_number_of_vertices(), 1U);

    const auto &v = dg.get_vertex(v_id);

    EXPECT_EQ(v.operation.jobId, FS::JobId(0U));
    EXPECT_EQ(v.operation.operationId, 0U);

    auto e = dg.add_edge(v_id,v_id,1);
    EXPECT_EQ(e.weight, 1U);
    EXPECT_EQ(e.src, v_id);
    EXPECT_EQ(e.dst, v_id);

    ASSERT_TRUE(v.get_incoming_edges().find(v_id) != v.get_incoming_edges().end());
    ASSERT_EQ(v.get_incoming_edges().size(), 1U);

    ASSERT_EQ(v.get_outgoing_edge(v_id), e);
    ASSERT_EQ(v.get_outgoing_edges().size(), 1U);
    ASSERT_EQ(v.get_outgoing_edges().at(0), e.weight);
}

TEST(GRAPH, SmallGraphWithNoEdges) {
    delayGraph dg;

    const unsigned int numNodes = 5U;
    for(unsigned int i = 0; i < numNodes; i++) {
        EXPECT_EQ(dg.get_number_of_vertices(), i);
        dg.add_vertex(FS::JobId(i),i);
        EXPECT_EQ(dg.get_number_of_vertices(), i+1);
    }

    for (const auto &v : dg.get_vertices()) {
        EXPECT_TRUE(v.get_incoming_edges().empty());
    }
}

TEST(GRAPH, TwoVertexCycle) {
    delayGraph dg;
    auto vId1 = dg.add_vertex(FS::JobId(0U), 0U);
    auto vId2 = dg.add_vertex(FS::JobId(0U), 1U);
    dg.add_edge(vId1, vId2, 1);

    auto &v1 = dg.get_vertex(vId1);
    auto &v2 = dg.get_vertex(vId2);

    EXPECT_TRUE(v1.get_incoming_edges().empty());
    EXPECT_FALSE(v2.get_incoming_edges().empty());

    dg.add_edge(v2, v1, 1);
    EXPECT_FALSE(v1.get_incoming_edges().empty());
    EXPECT_FALSE(v2.get_incoming_edges().empty());
}


TEST(GRAPH, SmallTree) {
    delayGraph dg;

    const int numNodes = 8;
    for(unsigned int i = 0; i < numNodes; i++) {
        dg.add_vertex(FS::JobId(i), FS::OperationId(i));
    }
    auto &vertices = dg.get_vertices();
    EXPECT_TRUE(vertices.at(0).get_incoming_edges().empty());

    dg.add_edge(vertices.at(0), vertices.at(1), 1);
    dg.add_edge(vertices.at(0), vertices.at(2), 2);

    for (const auto &[src,w] : vertices.at(0).get_incoming_edges()) {
        fmt::print("-{}\n", dg.get_vertex(src).operation);
    }

    EXPECT_TRUE(vertices.at(0).get_incoming_edges().empty());

    dg.add_edge(vertices.at(1), vertices.at(3), 3);
    dg.add_edge(vertices.at(1), vertices.at(4), 4);

    dg.add_edge(vertices.at(2), vertices.at(5), 5);
    dg.add_edge(vertices.at(2), vertices.at(6), 6);

    EXPECT_TRUE(vertices.at(0).get_incoming_edges().empty());
    EXPECT_EQ(vertices.at(0).get_outgoing_edges().size(), 2U);

    EXPECT_EQ(vertices.at(1).get_incoming_edges().size(), 1U);
    EXPECT_EQ(vertices.at(1).get_outgoing_edges().size(), 2U);

    EXPECT_EQ(vertices.at(2).get_incoming_edges().size(), 1U);
    EXPECT_EQ(vertices.at(2).get_outgoing_edges().size(), 2U);

    EXPECT_EQ(vertices.at(3).get_incoming_edges().size(), 1U);
    EXPECT_EQ(vertices.at(3).get_outgoing_edges().size(), 0U);
    EXPECT_EQ(vertices.at(4).get_incoming_edges().size(), 1U);
    EXPECT_EQ(vertices.at(4).get_outgoing_edges().size(), 0U);
    EXPECT_EQ(vertices.at(5).get_incoming_edges().size(), 1U);
    EXPECT_EQ(vertices.at(5).get_outgoing_edges().size(), 0U);
    EXPECT_EQ(vertices.at(6).get_incoming_edges().size(), 1U);
    EXPECT_EQ(vertices.at(6).get_outgoing_edges().size(), 0U);
}


TEST(GRAPH, JobSearch) {
    delayGraph dg;

    dg.add_vertex(FS::JobId(0U), FS::OperationId(0U));
    dg.add_vertex(FS::JobId(1U), FS::OperationId(1U));
    dg.add_vertex(FS::JobId(1U), FS::OperationId(2U));
    dg.add_vertex(FS::JobId(2U), FS::OperationId(1U));
    dg.add_vertex(FS::JobId(2U), FS::OperationId(2U));
    dg.add_vertex(FS::JobId(2U), FS::OperationId(3U));

    EXPECT_EQ(dg.get_vertices(FS::JobId(0)).size(), 1U);
    EXPECT_EQ(dg.get_vertices(FS::JobId(1)).size(), 2U);
    EXPECT_EQ(dg.get_vertices(FS::JobId(2)).size(), 3U);
    EXPECT_THROW((void)dg.get_vertices(FS::JobId(3)), FmsSchedulerException);

    EXPECT_EQ(dg.get_vertices({FS::JobId(0), FS::JobId(1), FS::JobId(2)}).size(), 6U);
    EXPECT_EQ(dg.get_vertices({FS::JobId(1), FS::JobId(2)}).size(), 5U);
    EXPECT_EQ(dg.get_vertices({FS::JobId(0), FS::JobId(2)}).size(), 4U);
    EXPECT_THROW((void) dg.get_vertices({FS::JobId(0), FS::JobId(3)}), FmsSchedulerException);
}

TEST(GRAPH, Copy) {
    delayGraph dg;

    constexpr std::array<FORPFSSPSD::operation, 3> ops{
            {{FS::JobId(0), 0}, {FS::JobId(1), 1}, {FS::JobId(2), 2}}};
    std::array<DelayGraph::VertexID, 3> ids{};

    for (std::size_t i = 0; i < ops.size(); i++) {
        ids.at(i) = dg.add_vertex(ops.at(i));
    }
    dg.add_edge(ids[0], ids[1], 10);
    dg.add_edge(ids[1], ids[2], 20);

    delayGraph dg2 = dg;

    for (std::size_t i = 0; i < ops.size(); i++) {
        EXPECT_TRUE(dg2.has_vertex(ids.at(i)));
        const auto &v = dg2.get_vertex(ids.at(i));
        EXPECT_EQ(v.operation, dg.get_vertex(ids.at(i)).operation);
        EXPECT_EQ(v.operation, ops.at(i));
    }

    dg.add_edge(ids[2], ids[0], 30);
    EXPECT_FALSE(dg2.has_edge(ids[2], ids[0]));
    EXPECT_TRUE(dg.has_edge(ids[2], ids[0]));

    dg2.add_edge(ids[2], ids[1], 40);
    EXPECT_FALSE(dg.has_edge(ids[2], ids[1]));
    EXPECT_TRUE(dg2.has_edge(ids[2], ids[1]));
}

// NOLINTEND(*-magic-numbers)