#include <gtest/gtest.h>

#include <fms/cg/builder.hpp>
#include <fms/problem/operation.hpp>
#include <fms/problem/xml_parser.hpp>

using namespace fms;
using namespace fms::problem;
using namespace fms::cg;

using Op = Operation;

TEST(GraphBuilder, Simple) {
    const auto instance = FORPFSSPSDXmlParser("simple/0.xml").createFlowShop();
    const auto graph = cg::Builder::FORPFSSPSD(instance);

    const Op op1{problem::JobId{0}, problem::OperationId{0}};
    const Op op2{problem::JobId{0}, problem::OperationId{1}};

    EXPECT_EQ(graph.getEdge(op1, op2).weight, 30);
    EXPECT_EQ(graph.getEdge(op2, op1).weight, -30);
}

TEST(GraphBuilder, ModularSynthetic0) {
    // TODO: Reimplement this test with production lines

    // auto instance = FORPFSSPSDXmlParser("modular/synthetic/1/0.xml").createFlowShop();
    // instance.modularise();
    // const auto graph = cg::Builder::FORPFSSPSD(instance);

    // EXPECT_EQ(graph.getEdge(op{0, 0}, op{0, 1}).weight, 30);
    // EXPECT_EQ(graph.getEdge(op{0, 1}, op{0, 0}).weight, -30);

    // const auto& m1 = instance.getModule(0);
    // const auto m1Graph = cg::Builder::FORPFSSPSD(m1);

    // EXPECT_EQ(m1Graph.getEdge(op{0, 0}, op{0, 1}).weight, 30);
    // EXPECT_EQ(m1Graph.getEdge(op{0, 1}, op{0, 0}).weight, -30);
}
