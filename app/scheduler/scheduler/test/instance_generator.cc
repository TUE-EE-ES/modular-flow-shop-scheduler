#include "FORPFSSPSD/indices.hpp"
#include <FORPFSSPSD/FORPFSSPSD.h>
#include <FORPFSSPSD/operation.h>
#include <delayGraph/delayGraph.h>
#include <map>


using namespace FORPFSSPSD;

FORPFSSPSD::Instance createHomogeneousCase(delay load_time, delay print_1_time, delay print_2_time, delay unload_time, delay buffer_min, delay buffer_max, unsigned int n_pages) {
    std::map<operation, unsigned int> sheetSizes {};
    DefaultOperationsTime processingTimes{0};
    FORPFSSPSD::TimeBetweenOps setupTimes;
    FORPFSSPSD::TimeBetweenOps dueDates;
    FORPFSSPSD::JobOperations jobs;
    FORPFSSPSD::OperationMachineMap operationMachineMap;
    
    for(JobId i(0); i.value < n_pages; i++) {
        processingTimes.insert({i, 0}, load_time);
        processingTimes.insert({i, 1}, print_1_time);
        processingTimes.insert({i, 2}, print_2_time);
        processingTimes.insert({i, 3}, unload_time);
        jobs[i] = {{i, 0}, {i, 1}, {i, 2}, {i, 3}};
        operationMachineMap[{i, 0}] = static_cast<MachineId>(0);
        operationMachineMap[{i, 1}] = static_cast<MachineId>(1);
        operationMachineMap[{i, 2}] = static_cast<MachineId>(1);
        operationMachineMap[{i, 3}] = static_cast<MachineId>(2);

        sheetSizes[{i, 0}] = 0;
        sheetSizes[{i, 1}] = 0;
        sheetSizes[{i, 2}] = 0;
        sheetSizes[{i, 3}] = 0;

        setupTimes.insert({i, 1}, {i, 2}, buffer_min - print_1_time);
        dueDates.insert({i, 2}, {i, 1}, buffer_max);
    }

    return {
            "Homogeneous generated case",
            std::move(jobs),
            std::move(operationMachineMap),
            processingTimes,
            DefaultTimeBetweenOps{0},
            std::move(setupTimes),
            TimeBetweenOps{},
            std::move(dueDates),
            JobsTime{},
            sheetSizes,
            0,
            0,
    };
}
