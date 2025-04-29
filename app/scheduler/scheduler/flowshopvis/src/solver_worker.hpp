//
// Created by Tristan van Triest on 31/05/2024.
//

#ifndef FMS_SCHEDULER_WORKER_H
#define FMS_SCHEDULER_WORKER_H

#include <fms/solvers/dd.hpp>

#include <QMutex>
#include <QThread>

// NOLINTBEGIN(readability-redundant-access-specifiers)

namespace FlowShopVis {

class SolverWorker : public QThread {
    Q_OBJECT

public:
    using PartialPathT = std::unordered_map<fms::dd::VertexId, fms::dd::MachinesSequences>;
    using TimesT = std::unordered_map<fms::dd::VertexId, std::vector<fms::algorithms::paths::PathTimes>>;

    explicit SolverWorker(QObject *parent = nullptr) : QThread(parent) {}

    void setArgs(fms::cli::CLIArgs args) { m_args = std::move(args); }

    [[nodiscard]] const auto &getArgs() const { return m_args; }

    void runDDSolver();

    void setData(fms::solvers::SolverDataPtr &&data);

    [[nodiscard]] fms::solvers::SolverDataPtr getData();

public slots:
    void run() override;

signals:
    void resultReady();
    void errorOccurred(const QString &error);
    void iteration(std::size_t iteration);

    void parsing(std::size_t progress);

private:
    fms::cli::CLIArgs m_args;

    fms::solvers::ResumableSolverOutput
    solveDDWrap(const fms::cli::CLIArgs &args,
                fms::problem::Instance &instance,
                fms::solvers::dd::DDSolverDataPtr dataOld);

    fms::solvers::SolverDataPtr m_solverData;

    QMutex m_mutex;
};

} // namespace FlowShopVis

// NOLINTEND(readability-redundant-access-specifiers)

#endif // FMS_SCHEDULER_WORKER_H
