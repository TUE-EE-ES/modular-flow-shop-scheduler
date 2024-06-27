#include <gtest/gtest.h>

#include <FORPFSSPSD/operation.h>
#include <FORPFSSPSD/xmlParser.h>
#include <delayGraph/builder.hpp>

using namespace FORPFSSPSD;

using op = operation;

TEST(GraphBuilder, Simple) {
    const auto instance = FORPFSSPSDXmlParser("simple/0.xml").createFlowShop();
    const auto graph = DelayGraph::Builder::FORPFSSPSD(instance);

    const op op1{FS::JobId{0}, FS::OperationId{0}};
    const op op2{FS::JobId{0}, FS::OperationId{1}};

    EXPECT_EQ(graph.get_edge(op1, op2).weight, 30);
    EXPECT_EQ(graph.get_edge(op2, op1).weight, -30);
}

TEST(GraphBuilder, ModularSynthetic0) {
    // TODO: Reimplement this test with production lines
    
    // auto instance = FORPFSSPSDXmlParser("modular/synthetic/1/0.xml").createFlowShop();
    // instance.modularise();
    // const auto graph = DelayGraph::Builder::FORPFSSPSD(instance);

    // EXPECT_EQ(graph.get_edge(op{0, 0}, op{0, 1}).weight, 30);
    // EXPECT_EQ(graph.get_edge(op{0, 1}, op{0, 0}).weight, -30);

    // const auto& m1 = instance.getModule(0);
    // const auto m1Graph = DelayGraph::Builder::FORPFSSPSD(m1);

    // EXPECT_EQ(m1Graph.get_edge(op{0, 0}, op{0, 1}).weight, 30);
    // EXPECT_EQ(m1Graph.get_edge(op{0, 1}, op{0, 0}).weight, -30);
}
