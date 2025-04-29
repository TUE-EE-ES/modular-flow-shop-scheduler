#include "solver_worker.hpp"

#include <fms/problem/xml_parser.hpp>
#include <fms/scheduler.hpp>
#include <fms/utils/pointers.hpp>

#include <QMutexLocker>
#include <QVariantMap>

namespace DDS = fms::solvers::dd;
using namespace std::chrono_literals;

void FlowShopVis::SolverWorker::run() { runDDSolver(); }

void FlowShopVis::SolverWorker::runDDSolver() {
    try {
        // Copy args to avoid race conditions
        auto args = m_args;
        // args.timeOut = 1000ms;
        fms::problem::FORPFSSPSDXmlParser parser(args.inputFile);
        args.algorithmOptions.emplace_back(DDS::kStoreHistory);
        fms::problem::Instance instance = fms::Scheduler::loadFlowShopInstance(args, parser);

        QMutexLocker locker(&m_mutex);
        auto data = fms::solvers::castSolverData<DDS::DDSolverData>(std::move(m_solverData));
        locker.unlock();

        auto [solutions, dataJSON, newData] = solveDDWrap(args, instance, std::move(data));
        data = fms::solvers::castSolverData<DDS::DDSolverData>(std::move(newData));

        locker.relock();
        m_solverData = std::move(data);
        emit resultReady();

        // emit resultReady(nodesPartialJson, allEdgesJson, statesPerDepthJson, opsCountJson);
    } catch (const std::exception &e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

fms::solvers::ResumableSolverOutput
FlowShopVis::SolverWorker::solveDDWrap(const fms::cli::CLIArgs &args,
                                       fms::problem::Instance &instance,
                                       DDS::DDSolverDataPtr dataOld) {
    auto data = DDS::initialize(args, instance, std::move(dataOld));

    // Count iterations
    std::uint32_t iterations = 0;
    constexpr const std::uint32_t kIterationUpdate = 10;

    while (!DDS::shouldStop(*data, args, iterations)) {
        DDS::singleIteration(*data, instance);

        if (iterations % kIterationUpdate == 0) {
            emit iteration(iterations);
        }
        ++iterations;
    }
    emit iteration(iterations);

    return DDS::solveTerminate(std::move(data));
}

void FlowShopVis::SolverWorker::setData(fms::solvers::SolverDataPtr &&data) {
    QMutexLocker locker(&m_mutex);
    m_solverData = std::move(data);
}

fms::solvers::SolverDataPtr FlowShopVis::SolverWorker::getData() {
    QMutexLocker locker(&m_mutex);
    return std::move(m_solverData);
}
