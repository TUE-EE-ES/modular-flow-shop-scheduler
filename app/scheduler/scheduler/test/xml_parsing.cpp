#include <gtest/gtest.h>

#include <fms/problem/xml_parser.hpp>

#include <array>

using namespace fms;
using namespace fms::problem;

// NOLINTBEGIN(*-magic-numbers)

TEST(XML, Simple) {
    FORPFSSPSDXmlParser parser("simple/0.xml");
    ASSERT_EQ(parser.getFileType(), FORPFSSPSDXmlParser::FileType::SHOP);

    std::vector<Operation> ops;
    for (JobId j(0); j < JobId(5); ++j) {
        for (OperationId o(0); o < OperationId(4); ++o) {
            ops.emplace_back(j, o);
        }
    }

    const auto instance = parser.createFlowShop();

    ASSERT_EQ(instance.getNumberOfJobs(), 5);
    ASSERT_EQ(instance.jobs().size(), 5);
    ASSERT_EQ(instance.getNumberOfOperationsPerJob(), 4);

    for (const auto &[job, ops] : instance.jobs()) {
        ASSERT_EQ(ops.size(), 4);
    }

    for (const auto &[jobId, _] : instance.jobs()) {
        EXPECT_EQ(instance.machineMapping().at(ops[0]), static_cast<MachineId>(0));
        EXPECT_EQ(instance.machineMapping().at(ops[1]), static_cast<MachineId>(1));
        EXPECT_EQ(instance.machineMapping().at(ops[2]), static_cast<MachineId>(1));
        EXPECT_EQ(instance.machineMapping().at(ops[3]), static_cast<MachineId>(2));
    }

    EXPECT_EQ(instance.processingTimes(ops[0]), 30);
    EXPECT_EQ(instance.processingTimes(ops[1]), 30);
    EXPECT_EQ(instance.processingTimes(ops[5]), 30);
    EXPECT_EQ(instance.processingTimes(ops[6]), 30);

    EXPECT_EQ(instance.dueDatesIndep().getMaybe(ops[2], ops[1]), 1200);

    EXPECT_EQ(instance.setupTimesIndep(ops[1], ops[2]), 70);

    EXPECT_FALSE(instance.setupTimes().contains(ops[1], ops[2]));
    EXPECT_FALSE(instance.setupTimes().contains(ops[0], ops[1]));
    EXPECT_EQ(instance.setupTimes(ops[0], ops[4]), 20);
    EXPECT_EQ(instance.setupTimes(ops[2], ops[6]), 20);
    EXPECT_EQ(instance.setupTimes(ops[2], ops[5]), 100);
    EXPECT_EQ(instance.setupTimes().getDefaultValue(), 20);
}

TEST(XML, SimpleMultiPlexity) {
    FORPFSSPSDXmlParser parser("simple/1.xml");
    EXPECT_EQ(parser.getFileType(), FORPFSSPSDXmlParser::FileType::SHOP);

    const auto instance = parser.createFlowShop();

    const std::array<Operation, 7> ops{{{JobId(0), 0},
                                        {JobId(0), 1},
                                        {JobId(0), 2},
                                        {JobId(0), 3},
                                        {JobId(1), 0},
                                        {JobId(1), 2},
                                        {JobId(1), 3}}};

    EXPECT_EQ(instance.getNumberOfJobs(), 5);
    EXPECT_EQ(instance.jobs().size(), 5);
    EXPECT_EQ(instance.getNumberOfOperationsPerJob(), 4);
    
    EXPECT_EQ(instance.jobs().at(JobId(0)).size(), 4);
    EXPECT_EQ(instance.jobs().at(JobId(1)).size(), 3);

    EXPECT_EQ(instance.machineMapping().at(ops[0]), static_cast<MachineId>(0));
    EXPECT_EQ(instance.machineMapping().find({JobId(1), OperationId(1)}), instance.machineMapping().end());

    EXPECT_EQ(instance.processingTimes(ops[0]), 30);
    EXPECT_EQ(instance.processingTimes(ops[1]), 30);
    EXPECT_EQ(instance.processingTimes(ops[5]), 30);
    
    EXPECT_EQ(instance.dueDatesIndep().getMaybe(ops[2], ops[1]), 1200);

    EXPECT_EQ(instance.setupTimesIndep(ops[1], ops[2]), 70);

    EXPECT_FALSE(instance.setupTimes().contains(ops[0], ops[1]));
    EXPECT_FALSE(instance.setupTimes().contains(ops[1], ops[2]));
    EXPECT_EQ(instance.setupTimes(ops[0], ops[4]), 20);
    EXPECT_EQ(instance.setupTimes(ops[2], ops[5]), 20);
    EXPECT_EQ(instance.setupTimes().getDefaultValue(), 0);
}

TEST(XML, ProductionLine) {
    FORPFSSPSDXmlParser parser("modular/synthetic/1/0.xml");
    EXPECT_EQ(parser.getFileType(), FORPFSSPSDXmlParser::FileType::MODULAR);

    const auto instance = parser.createProductionLine();

    EXPECT_EQ(instance.getNumberOfJobs(), 5);
    EXPECT_EQ(instance.getNumberOfMachines(), 6);

    const auto &transfers = instance.getTransferConstraints();
    EXPECT_EQ(transfers.size(), 1);

    const auto idM1 = static_cast<ModuleId>(0);
    const auto idM2 = static_cast<ModuleId>(1);
    const auto &ids = instance.moduleIds();
    EXPECT_EQ(ids.size(), 2);
    EXPECT_EQ(ids.at(0), idM1);
    EXPECT_EQ(ids.at(1), idM2);

    const auto &t1 = transfers(idM1, idM2);
    const auto &tSetup = t1.setupTime;
    const auto &tDeadline = t1.dueDate;
    const auto &proc = instance[idM1].processingTimes();

    for (const auto &[job, ops] : instance[idM1].jobs()) {
        const auto &opLast = ops.back();
        EXPECT_EQ(tSetup(job), 100);
        EXPECT_EQ(tDeadline.at(job), 100 + proc(opLast));
    }

    EXPECT_EQ(tSetup(JobId(100)), 0);
    EXPECT_EQ(tDeadline.find(JobId(100)), tDeadline.end());

    // Check that job 1 of module 1 is simplex
    {
        const auto &jobs = instance[idM1].jobs();
        const auto j1Id = static_cast<JobId>(1);
        const auto &j1ops = jobs.at(j1Id);

        const std::array operationIds{0, 2, 3};
        EXPECT_EQ(j1ops.size(), operationIds.size());
        for (size_t i = 0; i < operationIds.size(); ++i) {
            EXPECT_EQ(j1ops.at(i), problem::Operation(j1Id, operationIds.at(i)));
        }
    }

    // Check that job 2 of module 2 is duplex
    {
        const auto &jobs = instance[idM2].jobs();
        const auto j2Id = static_cast<JobId>(2);
        const auto &j2ops = jobs.at(j2Id);

        const std::array operationIds{0, 1, 2, 3};
        EXPECT_EQ(j2ops.size(), operationIds.size());
        for (size_t i = 0; i < operationIds.size(); ++i) {
            EXPECT_EQ(j2ops.at(i), problem::Operation(j2Id, operationIds.at(i)));
        }
    }
}

TEST(XML, ProductionLineNonTerminating) {
    FORPFSSPSDXmlParser parser("modular/synthetic/non-terminating/problem.xml");
    EXPECT_EQ(parser.getFileType(), FORPFSSPSDXmlParser::FileType::MODULAR);

    const auto instance = parser.createProductionLine();

    EXPECT_EQ(instance.getNumberOfModules(), 3);
    EXPECT_EQ(instance.getNumberOfJobs(), 3);
    EXPECT_EQ(instance.getNumberOfMachines(), 3);

    const auto &transfers = instance.getTransferConstraints();
    EXPECT_EQ(transfers.size(), 2);

    const auto idM2 = static_cast<ModuleId>(1);
    const auto &module2 = instance[idM2];

    // Check that the "special" edge is there
    {
        const auto value = module2.query({JobId(1), OperationId(0)}, {JobId(2), OperationId(1)});
        EXPECT_EQ(value, 20);
    }
}

// NOLINTEND(*-magic-numbers)
