import argparse
from json.encoder import INFINITY
from pathlib import Path
from typing import Optional
from collections import defaultdict

import docplex.cp.model as cp
import docplex.cp.solver.cpo_callback as cpo_callback
import cbor2

from model import Model
from utils.flowshop import ModuleId, JobId, OperationId

PATH_ROOT = Path(__file__).parent.parent


class intermediateSolverStatus(cpo_callback.CpoCallback):
    def __init__(
        self,
    ):
        self.anytimeData = {"anytime-solutions": [], "anytime-bounds": []}

    def invoke(self, solver, event, sres):
        # TO DO: Currently updates for all events, try to update only when new solution found
        if sres.is_solution():
            self.anytimeData["anytime-solutions"].append(
                (sres.get_solver_infos().get_solve_time(), sres.get_objective_value())
            )
            self.anytimeData["anytime-bounds"].append(
                (sres.get_solver_infos().get_solve_time(), sres.get_objective_bound())
            )


class cpModelSAGPaper(Model):
    def __init__(
        self,
    ):
        super().__init__()
        self.model = cp.CpoModel()
        self.solutionCallback = intermediateSolverStatus()
        self.initial_solution = None  # should be aan array of start times
        self.ops = {}
        self.seq = {}

    def read_input(self, spec):
        self.input = self.extractProductionLine(spec)

    def jobOps(self, njobs, nops):
        return [(i, j) for j in range(nops) for i in range(njobs)]

    def make_model(self):
        ops = {}  # variables
        seq = {}  # machine sequences
        for module_id, fs in self.input.modules.items():
            ops[module_id] = {}
            seq[module_id] = {}
            tm = {}  # transition matrix

            for j in fs.jobs:
                for op in j.operations.values():
                    ops[module_id][op.idx] = cp.interval_var(
                        size=op.processing, name=f"O{module_id}_{j.idx}_{op.idx[1]}", optional=False
                    )

            for m_id, machine in fs.machines.items():
                op_ids = {op.idx: i for i, op in enumerate(machine.operations)}
                seq[module_id][m_id] = cp.sequence_var(
                    [ops[module_id][op.idx] for op in machine.operations],
                    types=[op_ids[op.idx] for op in machine.operations],
                    name=f"M{module_id}_{m_id}",
                )

                # sequence dependent setup time matrix
                tm_rows = []
                for op1 in machine.operations:
                    tm_rows.append([fs.query(op1.idx, op2.idx) for op2 in machine.operations])

                tm[m_id] = cp.transition_matrix(tm_rows)

            # -----------------------------------------------------------------------------
            # CONSTRAINTS
            # -----------------------------------------------------------------------------

            # C1: Ensure jobs operations are done in order
            for j in fs.jobs:
                op_list = list(j.operations.values())
                for op_prev_last, op_next_last in zip(op_list, op_list[1:]):
                    # Setup time
                    self.model.add(
                        cp.end_of(ops[module_id][op_prev_last.idx])
                        <= cp.start_of(ops[module_id][op_next_last.idx])
                    )

            # C2 and C3: Overlap
            for m_id, machine in fs.machines.items():
                seq_machine = seq[module_id][m_id]

                # no overlap same machine
                self.model.add(cp.no_overlap(seq_machine))

                # no overlap same machine and sequence dependent setup time
                self.model.add(cp.no_overlap(seq_machine, tm[m_id], is_direct=True))

            # C4: Finish adding sequence independent setup times
            for op1, op2 in fs.setupIndepDict.keys():
                self.model.add(
                    cp.end_of(ops[module_id][op1]) + fs.query(op1, op2)
                    <= cp.start_of(ops[module_id][op2])
                )

            # C5: Due dates
            for (op1, op2), value in fs.dueIndepDict.items():
                self.model.add(
                    cp.start_of(ops[module_id][op2]) + value >= cp.start_of(ops[module_id][op1])
                )

            for j_prev, j_next in zip(fs.jobs, fs.jobs[1:]):
                ops_prev = list(j_prev.operations.values())
                ops_next = list(j_next.operations.values())

                # C6: Fixed output order
                op_prev_last = ops_prev[-1].idx
                op_next_last = ops_next[-1].idx
                self.model.add(
                    cp.start_of(ops[module_id][op_prev_last])
                    <= cp.start_of(ops[module_id][op_next_last])
                )

                # C7: No overtaking
                for op_prev, op_prev_next in zip(ops_prev, ops_prev[1:]):
                    for op_next, op_next_next in zip(ops_next, ops_next[1:]):
                        if (
                            op_prev.machine != op_next.machine
                            or op_prev_next.machine != op_next_next.machine
                        ):
                            continue

                        self.model.add(
                            cp.if_then(
                                cp.start_of(ops[module_id][op_prev.idx])
                                < cp.start_of(ops[module_id][op_next.idx]),
                                cp.start_of(ops[module_id][op_prev_next.idx])
                                < cp.start_of(ops[module_id][op_next_next.idx]),
                            )
                        )
        # -----------------------------------------------------------------------------
        # Transfer constraints
        # -----------------------------------------------------------------------------

        # CT1: Setup times
        for (module_id_prev, module_id_next), jobs in self.input.setup.items():
            fs_prev = self.input.modules[module_id_prev]
            fs_next = self.input.modules[module_id_next]

            for job_id, value in jobs.items():
                op_last = list(fs_prev.jobs_dict[job_id].operations.values())[-1]
                op_first = list(fs_next.jobs_dict[job_id].operations.values())[0]

                self.model.add(
                    cp.end_of(ops[module_id_prev][op_last.idx]) + value
                    <= cp.start_of(ops[module_id_next][op_first.idx])
                )

        # CT2: Due dates
        for (module_id_prev, module_id_next), jobs in self.input.due.items():
            fs_prev = self.input.modules[module_id_prev]
            fs_next = self.input.modules[module_id_next]

            for job_id, value in jobs.items():
                op_last = list(fs_prev.jobs_dict[job_id].operations.values())[-1]
                op_first = list(fs_next.jobs_dict[job_id].operations.values())[0]

                self.model.add(
                    cp.end_of(ops[module_id_prev][op_last.idx]) + value
                    >= cp.start_of(ops[module_id_next][op_first.idx])
                )

        # Objective: last operation of last module
        module_id_last = max(self.input.modules.keys())
        fs_last = self.input.modules[module_id_last]
        op_last = list(fs_last.jobs[-1].operations.values())[-1]
        self.model.add(cp.minimize(cp.end_of(ops[module_id_last][op_last.idx])))

        self.ops = ops
        self.seq = seq

        # add callback to monitor solve
        # self.model.add_solver_callback(self.solutionCallback)

    def set_solution(self, path_solution: Path):
        with open(path_solution, "rb") as f:
            data = cbor2.load(f)

        start_point = self.model.create_empty_solution()
        solution = data["solution"]
        for module_id_str, jobs in solution.items():
            module_id = ModuleId(int(module_id_str))
            for job_id_str, ops in jobs.items():
                job_id = JobId(int(job_id_str))
                for op_id_str, start in ops.items():
                    op_id = (job_id, OperationId(int(op_id_str)))
                    op = self.ops[module_id][op_id]
                    start_point.add_interval_var_solution(op, presence=True, start=start)
        self.model.set_starting_point(start_point)

    def export(self, filename: Path):
        self.model.export_model(str(filename.with_suffix(".cpo")))

    def solve(self, timeout: float = INFINITY) -> Optional[cp.CpoSolveResult]:
        return self.model.solve(
            TimeLimit=timeout,
            TimeMode="CPUTime",
            SearchType="IterativeDiving",
            OptimalityTolerance=0.0000000000001,
            Workers=1,
        )

    def log_results(self, solution: Optional[cp.CpoSolveResult], outputFile: Path, timeout):
        # -----------------------------------------------------------------------------
        # Print Solution
        # -----------------------------------------------------------------------------
        timing = defaultdict(lambda: defaultdict(dict))
        if solution is not None:
            print("Solution:")
            for module_id, ops in self.ops.items():
                print(f"Module {module_id}:")
                ops_sorted = sorted(ops.items(), key=lambda x: x[0])
                first_id = None
                for op_id, op in ops_sorted:
                    var_solution = solution.get_var_solution(op)
                    if var_solution is None:
                        continue

                    if first_id is None:
                        first_id = op_id[0]

                    if op_id[0] != first_id:
                        print()
                    print(f"{op_id[0]:>3}_{op_id[1]}: {int(var_solution.start):>10}", end="   ")
                    first_id = op_id[0]
                    timing[str(module_id)][str(op_id[0])][str(op_id[1])] = int(var_solution.start)
                print()

        njobs = len(self.input.module_last.jobs)
        if solution is not None:
            resultAttributes = {
                "minMakespan": solution.get_objective_value(),
                "lowerBound": solution.get_objective_bound(),
                "totalTime": solution.get_process_infos().get_total_solve_time(),
                "timePerJob": (solution.get_process_infos().get_total_solve_time() or 0) / njobs,
                "timeOutValue": timeout,
                "optimalityGap": solution.get_objective_gap(),
                "optimal": solution.is_solution_optimal(),
                "solved": solution.is_solution(),
                "timeout": solution.is_solution_optimal(),
                "solution": timing,
            }
        else:
            resultAttributes = {
                "minMakespan": "inf",
                "lowerBound": "inf",
                "totalTime": timeout,
                "timePerJob": timeout / njobs,
                "timeOutValue": timeout,
                "optimalityGap": "inf",
                "solved": False,
                "solution": timing,
                "error": "time-out",
                "timeout": True,
            }

        cbor_output = outputFile.with_suffix(outputFile.suffix + ".cbor")
        cbor_output.resolve().parent.mkdir(parents=True, exist_ok=True)
        with open(cbor_output, "wb") as fp:
            cbor2.dump(resultAttributes, fp)

        print(f"Stored result in {cbor_output}")


def main(args: dict):
    model = cpModelSAGPaper()
    print(f"Reading input from {args['input']}")
    model.read_input(args["input"])
    model.make_model()

    path_solution: Optional[Path] = args["solution"]
    if path_solution is not None:
        if not path_solution.exists():
            raise FileNotFoundError(f"Solution file {path_solution} does not exist")
        model.set_solution(path_solution)

    if args["export"]:
        model.export(args["output"])

    timelimit = int(args["time_limit"])
    solution = model.solve(timelimit)
    model.log_results(solution, args["output"], timelimit)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="CP model for scheduling of production lines")
    parser.add_argument("input", type=Path, help="input file")
    parser.add_argument("output", type=Path, help="output file")
    parser.add_argument(
        "--time-limit",
        type=int,
        default="600",
        help="time limit in seconds",
    )

    parser.add_argument(
        "--export",
        action="store_true",
        help="Export generated model",
        default=False,
    )

    parser.add_argument(
        "--solution",
        type=Path,
        help="CBOR solution file to read and verify",
        default=None,
    )

    args = parser.parse_args()
    main(vars(args))
