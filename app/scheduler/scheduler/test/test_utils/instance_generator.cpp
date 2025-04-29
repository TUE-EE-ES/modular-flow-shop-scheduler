#include <fms/cg/constraint_graph.hpp>
#include <fms/problem/flow_shop.hpp>
#include <fms/problem/indices.hpp>
#include <fms/problem/operation.hpp>
#include <map>

using namespace fms;

problem::Instance createHomogeneousCase(delay load_time,
                                        delay print_1_time,
                                        delay print_2_time,
                                        delay unload_time,
                                        delay buffer_min,
                                        delay buffer_max,
                                        unsigned int n_pages) {
    problem::OperationSizes sheetSizes{0};
    problem::DefaultOperationsTime processingTimes{0};
    problem::TimeBetweenOps setupTimes;
    problem::TimeBetweenOps dueDates;
    problem::JobOperations jobs;
    problem::OperationMachineMap operationMachineMap;

    for (problem::JobId i(0); i.value < n_pages; i++) {
        processingTimes.insert({i, 0}, load_time);
        processingTimes.insert({i, 1}, print_1_time);
        processingTimes.insert({i, 2}, print_2_time);
        processingTimes.insert({i, 3}, unload_time);
        jobs[i] = {{i, 0}, {i, 1}, {i, 2}, {i, 3}};
        operationMachineMap[{i, 0}] = static_cast<problem::MachineId>(0);
        operationMachineMap[{i, 1}] = static_cast<problem::MachineId>(1);
        operationMachineMap[{i, 2}] = static_cast<problem::MachineId>(1);
        operationMachineMap[{i, 3}] = static_cast<problem::MachineId>(2);

        sheetSizes.insert({i, 0}, 0);
        sheetSizes.insert({i, 1}, 0);
        sheetSizes.insert({i, 2}, 0);
        sheetSizes.insert({i, 3}, 0);

        setupTimes.insert({i, 1}, {i, 2}, buffer_min - print_1_time);
        dueDates.insert({i, 2}, {i, 1}, buffer_max);
    }

    return {
            "Homogeneous generated case",
            std::move(jobs),
            std::move(operationMachineMap),
            processingTimes,
            problem::DefaultTimeBetweenOps{0},
            std::move(setupTimes),
            problem::TimeBetweenOps{},
            std::move(dueDates),
            problem::JobsTime{},
            std::move(sheetSizes),
            0,
    };
}
