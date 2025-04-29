#include <gtest/gtest.h>

#include <fms/cg/constraint_graph.hpp>

#include <array>

using namespace fms;
using namespace fms::cg;

// NOLINTBEGIN(*-magic-numbers)

TEST(Graph, EmptyGraph) {
    ConstraintGraph dg;

    EXPECT_EQ(dg.getNumberOfVertices(), 0U);
    EXPECT_TRUE(dg.getVertices().empty());
}

TEST(Graph, SingleVertexGraph) {
    ConstraintGraph dg;
    VertexId v_id = dg.addVertex(problem::JobId(0U), 0U);
    EXPECT_EQ(dg.getNumberOfVertices(), 1U);

    const auto &v = dg.getVertex(v_id);

    EXPECT_EQ(v.operation.jobId, problem::JobId(0U));
    EXPECT_EQ(v.operation.operationId, 0U);

    auto e = dg.addEdge(v_id, v_id, 1);
    EXPECT_EQ(e.weight, 1U);
    EXPECT_EQ(e.src, v_id);
    EXPECT_EQ(e.dst, v_id);

    ASSERT_TRUE(v.getIncomingEdges().find(v_id) != v.getIncomingEdges().end());
    ASSERT_EQ(v.getIncomingEdges().size(), 1U);

    ASSERT_EQ(v.getOutgoingEdge(v_id), e);
    ASSERT_EQ(v.getOutgoingEdges().size(), 1U);
    ASSERT_EQ(v.getOutgoingEdges().at(0), e.weight);
}

TEST(Graph, SmallGraphWithNoEdges) {
    ConstraintGraph dg;

    const unsigned int numNodes = 5U;
    for (unsigned int i = 0; i < numNodes; i++) {
        EXPECT_EQ(dg.getNumberOfVertices(), i);
        dg.addVertex(problem::JobId(i), i);
        EXPECT_EQ(dg.getNumberOfVertices(), i + 1);
    }

    for (const auto &v : dg.getVertices()) {
        EXPECT_TRUE(v.getIncomingEdges().empty());
    }
}

TEST(Graph, TwoVertexCycle) {
    ConstraintGraph dg;
    auto vId1 = dg.addVertex(problem::JobId(0U), 0U);
    auto vId2 = dg.addVertex(problem::JobId(0U), 1U);
    dg.addEdge(vId1, vId2, 1);

    auto &v1 = dg.getVertex(vId1);
    auto &v2 = dg.getVertex(vId2);

    EXPECT_TRUE(v1.getIncomingEdges().empty());
    EXPECT_FALSE(v2.getIncomingEdges().empty());

    dg.addEdge(v2, v1, 1);
    EXPECT_FALSE(v1.getIncomingEdges().empty());
    EXPECT_FALSE(v2.getIncomingEdges().empty());
}

TEST(Graph, SmallTree) {
    ConstraintGraph dg;

    const int numNodes = 8;
    for (unsigned int i = 0; i < numNodes; i++) {
        dg.addVertex(problem::JobId(i), problem::OperationId(i));
    }
    auto &vertices = dg.getVertices();
    EXPECT_TRUE(vertices.at(0).getIncomingEdges().empty());

    dg.addEdge(vertices.at(0), vertices.at(1), 1);
    dg.addEdge(vertices.at(0), vertices.at(2), 2);

    for (const auto &[src, w] : vertices.at(0).getIncomingEdges()) {
        fmt::print("-{}\n", dg.getVertex(src).operation);
    }

    EXPECT_TRUE(vertices.at(0).getIncomingEdges().empty());

    dg.addEdge(vertices.at(1), vertices.at(3), 3);
    dg.addEdge(vertices.at(1), vertices.at(4), 4);

    dg.addEdge(vertices.at(2), vertices.at(5), 5);
    dg.addEdge(vertices.at(2), vertices.at(6), 6);

    EXPECT_TRUE(vertices.at(0).getIncomingEdges().empty());
    EXPECT_EQ(vertices.at(0).getOutgoingEdges().size(), 2U);

    EXPECT_EQ(vertices.at(1).getIncomingEdges().size(), 1U);
    EXPECT_EQ(vertices.at(1).getOutgoingEdges().size(), 2U);

    EXPECT_EQ(vertices.at(2).getIncomingEdges().size(), 1U);
    EXPECT_EQ(vertices.at(2).getOutgoingEdges().size(), 2U);

    EXPECT_EQ(vertices.at(3).getIncomingEdges().size(), 1U);
    EXPECT_EQ(vertices.at(3).getOutgoingEdges().size(), 0U);
    EXPECT_EQ(vertices.at(4).getIncomingEdges().size(), 1U);
    EXPECT_EQ(vertices.at(4).getOutgoingEdges().size(), 0U);
    EXPECT_EQ(vertices.at(5).getIncomingEdges().size(), 1U);
    EXPECT_EQ(vertices.at(5).getOutgoingEdges().size(), 0U);
    EXPECT_EQ(vertices.at(6).getIncomingEdges().size(), 1U);
    EXPECT_EQ(vertices.at(6).getOutgoingEdges().size(), 0U);
}

TEST(Graph, JobSearch) {
    ConstraintGraph dg;

    dg.addVertex(problem::JobId(0U), problem::OperationId(0U));
    dg.addVertex(problem::JobId(1U), problem::OperationId(1U));
    dg.addVertex(problem::JobId(1U), problem::OperationId(2U));
    dg.addVertex(problem::JobId(2U), problem::OperationId(1U));
    dg.addVertex(problem::JobId(2U), problem::OperationId(2U));
    dg.addVertex(problem::JobId(2U), problem::OperationId(3U));

    EXPECT_EQ(dg.getVertices(problem::JobId(0)).size(), 1U);
    EXPECT_EQ(dg.getVertices(problem::JobId(1)).size(), 2U);
    EXPECT_EQ(dg.getVertices(problem::JobId(2)).size(), 3U);
    EXPECT_THROW((void)dg.getVertices(problem::JobId(3)), FmsSchedulerException);

    EXPECT_EQ(dg.getVertices({problem::JobId(0), problem::JobId(1), problem::JobId(2)}).size(), 6U);
    EXPECT_EQ(dg.getVertices({problem::JobId(1), problem::JobId(2)}).size(), 5U);
    EXPECT_EQ(dg.getVertices({problem::JobId(0), problem::JobId(2)}).size(), 4U);
    EXPECT_THROW((void)dg.getVertices({problem::JobId(0), problem::JobId(3)}), FmsSchedulerException);
}

TEST(Graph, Copy) {
    ConstraintGraph dg;

    constexpr std::array<problem::Operation, 3> ops{
            {{problem::JobId(0), 0}, {problem::JobId(1), 1}, {problem::JobId(2), 2}}};
    std::array<cg::VertexId, 3> ids{};

    for (std::size_t i = 0; i < ops.size(); i++) {
        ids.at(i) = dg.addVertex(ops.at(i));
    }
    dg.addEdge(ids[0], ids[1], 10);
    dg.addEdge(ids[1], ids[2], 20);

    ConstraintGraph dg2 = dg;

    for (std::size_t i = 0; i < ops.size(); i++) {
        EXPECT_TRUE(dg2.hasVertex(ids.at(i)));
        const auto &v = dg2.getVertex(ids.at(i));
        EXPECT_EQ(v.operation, dg.getVertex(ids.at(i)).operation);
        EXPECT_EQ(v.operation, ops.at(i));
    }

    dg.addEdge(ids[2], ids[0], 30);
    EXPECT_FALSE(dg2.hasEdge(ids[2], ids[0]));
    EXPECT_TRUE(dg.hasEdge(ids[2], ids[0]));

    dg2.addEdge(ids[2], ids[1], 40);
    EXPECT_FALSE(dg.hasEdge(ids[2], ids[1]));
    EXPECT_TRUE(dg2.hasEdge(ids[2], ids[1]));
}

// NOLINTEND(*-magic-numbers)
