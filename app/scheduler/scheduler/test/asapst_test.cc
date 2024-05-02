#include <gtest/gtest.h>

#include <FORPFSSPSD/FORPFSSPSD.h>
#include <FORPFSSPSD/indices.hpp>
#include <FORPFSSPSD/xmlParser.h>
#include <delayGraph/builder.hpp>
#include <delayGraph/export_utilities.h>
#include <longest_path.h>
#include <negativecyclefinder.h>

using namespace DelayGraph;

namespace {
std::tuple<FORPFSSPSD::JobOperations, FORPFSSPSD::OperationMachineMap>
createDefaultOps(std::uint32_t numJobs, std::uint32_t numOpsPerJob) {
    FORPFSSPSD::JobOperations jobs;
    FORPFSSPSD::OperationMachineMap opMachineMap;
    for (std::uint32_t jobId = 0; jobId < numJobs; ++jobId) {
        FORPFSSPSD::OperationsVector jobOps;
        for (std::uint32_t opId = 0; opId < numOpsPerJob; ++opId) {
            FORPFSSPSD::operation op{jobId, opId};
            jobOps.push_back(op);
            opMachineMap.emplace(op, static_cast<FORPFSSPSD::MachineId>(0));
        }
        jobs.emplace(jobId, std::move(jobOps));
    }
    return {std::move(jobs), std::move(opMachineMap)};
}

auto buildGraph() {
    delayGraph dg;
    std::array<DelayGraph::VertexID, 3> ids{};
    const auto mSrc = static_cast<FORPFSSPSD::MachineId>(0);

    dg.add_source(mSrc);
    for (std::size_t i = 0; i < ids.size(); i++) {
        ids.at(i) = dg.add_vertex(static_cast<FORPFSSPSD::JobId>(i),
                                  static_cast<FORPFSSPSD::OperationId>(i));
    }
    return std::make_tuple(dg, mSrc, ids);
}
} // namespace

// NOLINTBEGIN(*-magic-numbers)

TEST(ASAPST, DummyFlowShop) {
    auto [jobs, opMachineMap] = createDefaultOps(4, 1);

    FORPFSSPSD::Instance f("dummy",
                           std::move(jobs),
                           std::move(opMachineMap),
                           {{}, 1},
                           {{}, 0},
                           {},
                           {},
                           {},
                           {},
                           {},
                           0,
                           0);

    auto dg = Builder::FORPFSSPSD(f);
    auto res = algorithm::LongestPath::initializeASAPST(dg);
    auto result = algorithm::LongestPath::computeASAPST(dg, res);

    DelayGraph::export_utilities::saveAsTikz(f, dg, "dummy.tex");
    DelayGraph::export_utilities::saveAsDot(dg, "dummy.dot");
    ASSERT_TRUE(result.positiveCycle.empty());
    EXPECT_EQ(res[0], 0);
    EXPECT_EQ(res[1], 0);
    EXPECT_EQ(res[2], 1);
    EXPECT_EQ(res[3], 2);
    EXPECT_EQ(res[4], 3);
}

TEST(ASAPST, TightDeadlinesFlowShop) {
    auto [jobs, opMachineMap] = createDefaultOps(4, 1);
    FORPFSSPSD::Instance f(
            "test",
            std::move(jobs),
            std::move(opMachineMap),
            {{{{0, 0}, 1}, {{1, 0}, 2}, {{2, 0}, 3}, {{3, 0}, 4}}, 1},
            {{}, 0},
            {},
            {},
            {{{{1, 0}, {{{0, 0}, 1}}}, {{2, 0}, {{{1, 0}, 2}}}, {{3, 0}, {{{2, 0}, 3}}}}},
            {},
            {},
            0,
            0);

    auto dg = Builder::FORPFSSPSD(f);
    auto res = algorithm::LongestPath::initializeASAPST(dg);
    auto result = algorithm::LongestPath::computeASAPST(dg, res);

    DelayGraph::export_utilities::saveAsTikz(f, dg, "small_tight_deadline.tex");
    ASSERT_TRUE(result.positiveCycle.empty());
    EXPECT_EQ(res[0], 0);
    EXPECT_EQ(res[1], 0);
    EXPECT_EQ(res[2], 1);
    EXPECT_EQ(res[3], 3);
    EXPECT_EQ(res[4], 6);
}

TEST(ASAPST, InfeasibleDeadlinesFlowShop) {
    auto [jobs, opMachineMap] = createDefaultOps(4, 1);
    FORPFSSPSD::Instance f("test",
                           std::move(jobs),
                           std::move(opMachineMap),
                           {{{{0, 0}, 1}, {{1, 0}, 2}, {{2, 0}, 3}, {{3, 0}, 4}}, 1},
                           {{}, 0},
                           {},
                           {},
                           {{{{1, 0}, {{{0, 0}, 0}}}}},
                           {},
                           {},
                           0,
                           0);

    auto dg = Builder::FORPFSSPSD(f);
    auto res = algorithm::LongestPath::initializeASAPST(dg);
    auto result = algorithm::LongestPath::computeASAPST(dg, res);

    DelayGraph::export_utilities::saveAsTikz(f, dg, "small_tight_deadline.tex");
    EXPECT_FALSE(result.positiveCycle.empty());
}

TEST(ASAPST, FixDeadlinesTest) {

    auto [jobs, opMachineMap] = createDefaultOps(4, 2);
    FORPFSSPSD::Instance f("fix_deadlines",
                           std::move(jobs),
                           std::move(opMachineMap),
                           {{{{0, 0}, 1}, {{1, 0}, 2}, {{2, 0}, 3}, {{3, 0}, 4}}, 1},
                           {{}, 0},
                           {},
                           {},
                           {{{{0, 1}, {{{0, 0}, 10}}},
                             {{1, 1}, {{{1, 0}, 10}}},
                             {{2, 1}, {{{2, 0}, 10}}},
                             {{3, 1}, {{{3, 0}, 10}}}}},
                           {},
                           {},
                           0,
                           0);

    auto dg = Builder::FORPFSSPSD(f);
    auto res = algorithm::LongestPath::initializeASAPST(dg);
    
    {
        auto result = algorithm::LongestPath::computeASAPST(dg, res);
        EXPECT_TRUE(result.positiveCycle.empty());
    }

    // fix deadlines and setup times for some part
    for (vertex &v : dg.get_vertices()) {
        if (v.operation.jobId >= 2) {
            continue;
        }
        for (auto &[dst, weight] : v.get_outgoing_edges()) {
            // set the weight of the edge to be equal to the difference of the ASAP times
            // previously calculated
            delay newWeight = res[dst] - res[v.id];
            ASSERT_GE(newWeight, weight);
            v.set_weight(dst, newWeight);
        }
    }

    // TODO: if in fixed area, do a feasibility check based on the calculated ASAPST
    auto e = dg.add_edge(dg.get_vertex({3, 0}), dg.get_vertex({2, 1}), 8);

    {
        auto result = algorithm::LongestPath::computeASAPST(
                dg, res, dg.cget_vertices(2), dg.cget_vertices(3));
        ASSERT_FALSE(result.positiveCycle.empty());
    }
    dg.add_edge(FORPFSSPSD::operation{3,0}, FORPFSSPSD::operation{2,1}, 3);
    res = algorithm::LongestPath::initializeASAPST(dg);
    {
        auto result = algorithm::LongestPath::computeASAPST(dg, res);
        EXPECT_TRUE(result.positiveCycle.empty());
    }
    EXPECT_EQ(res.back(), 10);

    dg.add_edge(FORPFSSPSD::operation{3, 0}, FORPFSSPSD::operation{2, 1}, 7);
    auto res2 = res;

    {
        auto result = algorithm::LongestPath::computeASAPST(
                dg, res2, dg.cget_vertices(1), dg.cget_vertices(2, 3));

        DelayGraph::export_utilities::saveAsTikz(f, dg, "infeasible_1.tex");
        algorithm::LongestPath::dumpToFile(dg, res2, "asapst_test.txt");

        ASSERT_TRUE(result.positiveCycle.empty());
    }

    const auto nofMachines = f.getNumberOfMachines();
    EXPECT_EQ(res2[nofMachines + 0], 0);
    EXPECT_EQ(res2[nofMachines + 1], 1);
    EXPECT_EQ(res2[nofMachines + 2], 1);
    EXPECT_EQ(res2[nofMachines + 3], 3);
    EXPECT_EQ(res2[nofMachines + 4], 3);
    EXPECT_EQ(res2[nofMachines + 5], 13);
    EXPECT_EQ(res2[nofMachines + 6], 6);
    EXPECT_EQ(res2[nofMachines + 7], 14);

    DelayGraph::export_utilities::saveAsTikz(f, dg, "fixed_deadlines.tex");
    DelayGraph::export_utilities::saveAsDot(dg, "fixed_deadlines.dot");

}

TEST(ASAPST, splitComputation) {
    auto [jobs, opMachineMap] = createDefaultOps(4, 2);

    FORPFSSPSD::Instance f("fix_deadlines",
                           std::move(jobs),
                           std::move(opMachineMap),
                           {{{{0, 0}, 1}, {{1, 0}, 2}, {{2, 0}, 3}, {{3, 0}, 4}}, 1},
                           {{}, 0},
                           {},
                           {},
                           {{{{0, 1}, {{{0, 0}, 10}}},
                             {{1, 1}, {{{1, 0}, 10}}},
                             {{2, 1}, {{{2, 0}, 10}}},
                             {{3, 1}, {{{3, 0}, 10}}}}},
                           {},
                           {},
                           0,
                           0);

    auto dg = Builder::FORPFSSPSD(f);
    auto res = algorithm::LongestPath::initializeASAPST(dg);
    {
        auto result = algorithm::LongestPath::computeASAPST(dg, res);
        EXPECT_TRUE(result.positiveCycle.empty());
    }

    // fix deadlines and setup times for some part
    for (vertex &v : dg.get_vertices()) {
        if (v.operation.jobId >= 2) {
            continue;
        }
        for (auto &[dst, weight] : v.get_outgoing_edges()) {
            // set the weight of the edge to be equal to the difference of the ASAP times
            // previously calculated
            delay newWeight = res[dst] - res[v.id];
            ASSERT_GE(newWeight, weight);
            v.set_weight(dst, newWeight);
        }
    }

    // TODO: if in fixed area, do a feasibility check based on the calculated ASAPST
    dg.add_edge(dg.get_vertex({3, 0}), dg.get_vertex({2, 1}), 8);
    {
        auto result = algorithm::LongestPath::computeASAPST(
                dg, res, dg.cget_vertices(2), dg.cget_vertices(3));
        EXPECT_FALSE(result.positiveCycle.empty());
    }

    dg.add_edge(FORPFSSPSD::operation{3,0}, FORPFSSPSD::operation{2,1}, 3);
    res = algorithm::LongestPath::initializeASAPST(dg);
    algorithm::LongestPath::computeASAPST(
            dg, res, {dg.get_vertex(FORPFSSPSD::operation{0, 0})}, dg.get_vertices(0, 1));
    {
        auto result = algorithm::LongestPath::computeASAPST(
                dg, res, dg.cget_vertices(1), dg.cget_vertices(2, 3));
        EXPECT_TRUE(result.positiveCycle.empty());
    }

    EXPECT_EQ(res.back(), 10);
}

TEST(ASAPST, longestCycleFeasible) {
    delayGraph dg;

    auto v0 = dg.add_source(static_cast<FORPFSSPSD::MachineId>(0U));
    auto v1 = dg.add_vertex(0U, 0U);
    std::cout << v1 << std::endl;
    auto v2 = dg.add_vertex(1U, 1U);
    auto v3 = dg.add_vertex(1U, 2U);
    auto v4 = dg.add_vertex(2U, 1U);
    auto v5 = dg.add_vertex(2U, 2U);
    auto v6 = dg.add_vertex(2U, 3U);
    dg.add_edge(v0,v1, 0);
    dg.add_edge(v1,v2, 1);
    dg.add_edge(v2,v3, 1);
    dg.add_edge(v3,v4, 1);
    dg.add_edge(v4,v5, 1);
    dg.add_edge(v5,v6, 1);
    dg.add_edge(v6,v1, -5);

    auto res = algorithm::LongestPath::initializeASAPST(dg);
    {
        auto result = algorithm::LongestPath::computeASAPST(dg, res);
        for(const auto & edge : result.positiveCycle) {
            std::cout << edge.src << "--(" << edge.weight << ")-->" << edge.dst << std::endl;
        }
        EXPECT_TRUE(result.positiveCycle.empty());
    }
}

TEST(ASAPST, longestCycleInfeasible) {
    delayGraph dg;

    auto v0 = dg.add_source(static_cast<FORPFSSPSD::MachineId>(0U));
    auto v1 = dg.add_vertex(0U, 0U);
    auto v2 = dg.add_vertex(1U, 1U);
    auto v3 = dg.add_vertex(1U, 2U);
    auto v4 = dg.add_vertex(2U, 1U);
    auto v5 = dg.add_vertex(2U, 2U);
    auto v6 = dg.add_vertex(2U, 3U);

    dg.add_edge(v0,v1, 0);
    dg.add_edge(v1,v2, 1);
    dg.add_edge(v2,v3, 1);
    dg.add_edge(v3,v4, 1);
    dg.add_edge(v4,v5, 1);
    dg.add_edge(v5,v6, 1);
    dg.add_edge(v6,v1, -4);

    auto res = algorithm::LongestPath::initializeASAPST(dg);
    auto result = algorithm::LongestPath::computeASAPST(dg, res);

    EXPECT_FALSE(result.positiveCycle.empty());
}

TEST(ASAPST, longestCycleInfeasibleWindowed) {
    delayGraph dg;

    auto v0 = dg.add_source(static_cast<FORPFSSPSD::MachineId>(0U));

    auto vId1 = dg.add_vertex(0U, 0U);
    auto vId2 = dg.add_vertex(1U, 1U);
    auto vId3 = dg.add_vertex(1U, 2U);
    auto vId4 = dg.add_vertex(2U, 1U);
    auto vId5 = dg.add_vertex(2U, 2U);
    auto vId6 = dg.add_vertex(2U, 3U);
    auto vId7 = dg.add_vertex(2U, 4U);
    auto vId8 = dg.add_vertex(2U, 5U);
    auto vId9 = dg.add_vertex(2U, 6U);

    auto &v1 = dg.get_vertex(vId1);
    auto &v2 = dg.get_vertex(vId2);
    auto &v3 = dg.get_vertex(vId3);
    auto &v4 = dg.get_vertex(vId4);
    auto &v5 = dg.get_vertex(vId5);
    auto &v6 = dg.get_vertex(vId6);
    auto &v7 = dg.get_vertex(vId7);
    auto &v8 = dg.get_vertex(vId8);
    auto &v9 = dg.get_vertex(vId9);

    dg.add_edge(dg.get_vertex(v0),v1, 0);
    dg.add_edge(v1,v2, 1);
    dg.add_edge(v2,v3, 1);
    dg.add_edge(v3,v4, 1);
    dg.add_edge(v4,v5, 1);
    dg.add_edge(v5,v6, 1);
    dg.add_edge(v6,v7, 1);
    dg.add_edge(v7,v8, 1);
    dg.add_edge(v8,v9, 1);
    dg.add_edge(v9,v5, -3);

    auto asapst = algorithm::LongestPath::initializeASAPST(dg);
    auto negativeCycle = algorithm::LongestPath::computeASAPST(dg, asapst, {v2}, {v3,v4,v5,v6});

    algorithm::LongestPath::computeASAPST(dg, asapst);
    {
        auto result = algorithm::LongestPath::computeASAPST(dg, asapst, {v2}, {v3,v4,v5,v6});
        // feasible (no deadline is violated in this case)
        EXPECT_TRUE(result.positiveCycle.empty());
    }
}

TEST(ASAPST, incrementalNoMiss) {
    auto [dg, mSrc, ids] = buildGraph();
    dg.add_edge(ids.at(1), ids.at(2), 100);

    auto ASAPST = algorithm::LongestPath::initializeASAPST(dg);
    for (const auto id : ids) {
        EXPECT_EQ(ASAPST[id], algorithm::LongestPath::kASAPStartValue);
    }

    edge e{dg.get_source(mSrc).id, ids.at(0), 0};
    bool res = algorithm::LongestPath::addOneEdgeIncrementalASAPST(dg, e, ASAPST);
    dg.add_edge(e);
    EXPECT_FALSE(res);
    EXPECT_EQ(ASAPST[ids.at(0)], 0);

    e = {ids.at(0), ids.at(1), 1};
    res = algorithm::LongestPath::addOneEdgeIncrementalASAPST(dg, e, ASAPST);
    EXPECT_FALSE(res);
    EXPECT_EQ(ASAPST[ids.at(1)], 1);
    EXPECT_EQ(ASAPST[ids.at(2)], 101);
}

TEST(ASAPST, incrementalMiss) {
    auto [dg, mSrc, ids] = buildGraph();
    dg.add_edge(ids.at(1), ids.at(2), 100);
    dg.add_edge(ids.at(2), ids.at(0), -100);

    auto ASAPST = algorithm::LongestPath::initializeASAPST(dg);
    for (const auto id : ids) {
        EXPECT_EQ(ASAPST[id], algorithm::LongestPath::kASAPStartValue);
    }

    edge e{dg.get_source(mSrc).id, ids.at(0), 0};
    bool res = algorithm::LongestPath::addOneEdgeIncrementalASAPST(dg, e, ASAPST);
    dg.add_edge(e);
    EXPECT_FALSE(res);
    EXPECT_EQ(ASAPST[ids.at(0)], 0);

    e = {ids.at(0), ids.at(1), 1};
    res = algorithm::LongestPath::addOneEdgeIncrementalASAPST(dg, e, ASAPST);
    EXPECT_TRUE(res);
}

TEST(ASAPST, incrementalMultipleForward) {
    auto [dg, mSrc, ids] = buildGraph();
    dg.add_edge(ids.at(1), ids.at(2), 100);

    auto ASAPST = algorithm::LongestPath::initializeASAPST(dg);
    for (const auto id : ids) {
        EXPECT_EQ(ASAPST[id], algorithm::LongestPath::kASAPStartValue);
    }

    DelayGraph::Edges edges{{dg.get_source(mSrc).id, ids.at(0), 0}, {ids.at(0), ids.at(1), 5}};
    bool res = algorithm::LongestPath::addEdgesIncrementalASAPST(dg, edges, ASAPST);
    EXPECT_FALSE(res);
    EXPECT_EQ(ASAPST[ids.at(0)], 0);
    EXPECT_EQ(ASAPST[ids.at(1)], 5);
    EXPECT_EQ(ASAPST[ids.at(2)], 105);

    dg.add_edges(edges);

    // Add positive cycle and expect it to fail
    edges = {{ids.at(2), ids.at(0), 10}};
    res = algorithm::LongestPath::addEdgesIncrementalASAPST(dg, edges, ASAPST);
    EXPECT_TRUE(res);
}

TEST(ASAPST, incrementalMultipleBackward) {
    auto [dg, mSrc, ids] = buildGraph();
    dg.add_edge(ids.at(1), ids.at(2), 100);

    auto ASAPST = algorithm::LongestPath::initializeASAPST(dg);
    for (const auto id : ids) {
        EXPECT_EQ(ASAPST[id], algorithm::LongestPath::kASAPStartValue);
    }

    DelayGraph::Edges edges{{ids.at(0), ids.at(1), 5}, {dg.get_source(mSrc).id, ids.at(0), 0}};
    bool resConst = algorithm::LongestPath::addEdgesIncrementalASAPSTConst(dg, edges, ASAPST);
    EXPECT_FALSE(resConst);
    bool res = algorithm::LongestPath::addEdgesIncrementalASAPST(dg, edges, ASAPST);
    EXPECT_FALSE(res);
    EXPECT_EQ(ASAPST[ids.at(0)], 0);
    EXPECT_EQ(ASAPST[ids.at(1)], 5);
    EXPECT_EQ(ASAPST[ids.at(2)], 105);

    dg.add_edges(edges);

    // Add positive cycle and expect it to fail
    edges = {{ids.at(2), ids.at(0), 10}};
    auto ASAPSTCopy = ASAPST;
    resConst = algorithm::LongestPath::addEdgesIncrementalASAPSTConst(dg, edges, ASAPSTCopy);
    EXPECT_TRUE(resConst);
    res = algorithm::LongestPath::addEdgesIncrementalASAPST(dg, edges, ASAPST);
    EXPECT_TRUE(res);
}

TEST(ASAPST, singleNode) {
    // Load the non-terminating case
    using namespace FORPFSSPSD;
    FORPFSSPSDXmlParser parser("modular/synthetic/non-terminating/problem.xml");

    auto instance = parser.createProductionLine();

    auto dg = Builder::FORPFSSPSD(instance[static_cast<ModuleId>(1)]);
    const operation opS{1, 1};
    const auto vId = dg.get_vertex_id(opS);
    auto res = algorithm::LongestPath::computeASAPSTFromNode(dg, vId);

    const auto k = algorithm::LongestPath::kASAPStartValue;
    const algorithm::PathTimes v{k, k, k, k, 0, 2, 3};
    EXPECT_EQ(res, v);
}

TEST(ASAPST, obtainCycle) {
    delayGraph dg;

    std::vector<DelayGraph::VertexID> ids;

    ids.push_back(dg.add_source(FORPFSSPSD::MachineId{0U}));
    ids.push_back(dg.add_vertex(0U, 0U));
    ids.push_back(dg.add_vertex(1U, 1U));
    ids.push_back(dg.add_vertex(2U, 2U));
    ids.push_back(dg.add_vertex(3U, 3U));

    // We create a positive cycle
    dg.add_edge(ids.at(0), ids.at(1), 1);
    dg.add_edge(ids.at(1), ids.at(2), 1);
    dg.add_edge(ids.at(2), ids.at(3), 1);
    dg.add_edge(ids.at(3), ids.at(1), 1);
    dg.add_edge(ids.at(3), ids.at(4), 1); // This edge is not part of the cycle

    // Obtain the longest path
    auto [infeasibleEdge, _] = algorithm::LongestPath::computeASAPST(dg);

    // We expect to find a positive cycle
    ASSERT_FALSE(infeasibleEdge.positiveCycle.empty());

    // Obtain the edges of the cycle
    auto cycleEdges = algorithm::LongestPath::getPositiveCycle(dg);

    // We expect to find a cycle of length 3
    EXPECT_EQ(cycleEdges.size(), 3);

    // We expect to find the edges (0,1), (1,2), (2,0) in any order
    DelayGraph::Edges expectedEdges = {
            {ids.at(1), ids.at(2), 1}, {ids.at(2), ids.at(3), 1}, {ids.at(3), ids.at(1), 1}};

    for (const auto &edge : cycleEdges) {
        auto it = std::find(expectedEdges.begin(), expectedEdges.end(), edge);
        EXPECT_NE(it, expectedEdges.end());
    }
}

// NOLINTEND(*-magic-numbers)
