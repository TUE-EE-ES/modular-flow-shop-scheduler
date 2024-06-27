"""This module provides a command-line interface for running the modfs tool.

The modfs tool was designed for generating and testing modular production lines.
Now it can only run production line files using the fms-scheduler tool. The generator
has been moved to a separate tool written in julia called FMSScripts.

This module defines the command-line interface for the modfs run tool. It uses the
`argparse` module to parse command-line arguments, and the `Executor` class to
run the modfs tool.

To use the modfs tool, simply call run `python -m modfs run`, passing in the
appropriate command-line arguments. For more information on the available
arguments, run `python -m modfs run --help`.
"""

import argparse
import shutil
from pathlib import Path

import modfs.utils as utils

from modfs.runner.tasks.runner import Executor

DEFAULT_ARGS = {
    "print_defaults": False,
    "scheduler": Path("./data/bin/scheduler"),
    "cp_solver": Path("./data/bin/cp-solver"),
    "out": Path("./data/run"),
    "in": Path("./data/gen"),
    "runs": 1,
    "debug": False,
    "run_all": False,
    "processes": None,
    "verbose": False,
    "algorithm": [],
    "modular_algorithm": ["cocktail"],
    "make_dirs_only": False,
    "time_out": 600,
}


def add_arguments(parser: argparse.ArgumentParser):
    """Add arguments to the parser."""
    parser.set_defaults(func=_main)

    parser.add_argument(
        "--print-defaults",
        help="Shows the default values used by the arguments",
        action="store_true",
    )
    parser.add_argument(
        "--out",
        "-o",
        help=(
            "Path to directory where to output all the files (will be created if it does not exist)"
        ),
        type=Path,
    )
    parser.add_argument(
        "--in", "-i", help="Path to directory where to find the files to be processed", type=Path
    )
    parser.add_argument("--runs", help="Number of times each problem will be evaluated", type=int)
    parser.add_argument(
        "--verbose",
        help="Show verbose information when running algorithm",
        action="store_true",
    )
    parser.add_argument(
        "--run-all",
        help="Run all elements, even if the result already exist. By default, "
        "existing results are ignored",
        action="store_true",
    )
    parser.add_argument(
        "--make-dirs-only",
        help="Only make directories, do not run the algorithm",
        action="store_true",
    )
    parser.add_argument(
        "--processes", help="Number of parallel processes to use when running", type=int
    )
    parser.add_argument(
        "--scheduler",
        help=(
            "Path to the FMS-scheduler executable usually called app. It is recommended to create a"
            " symlink to the default location"
        ),
        type=Path,
    )
    parser.add_argument(
        "--cp-solver",
        help="Path to the script that runs the python CP solver.",
    )
    parser.add_argument(
        "--algorithm",
        help="Algorithm used by the scheduler",
        action="extend",
        nargs="+",
        choices=["bhcs", "mdbhcs", "backtrack", "dd", "simple"],
    )
    parser.add_argument(
        "--modular-algorithm",
        help="Algorithm used by the modular constraint propagation",
        action="extend",
        nargs="+",
        choices=["broadcast", "cocktail", "constraint"],
    )

    parser.add_argument(
        "--time-out", help="Time out for the algorithm in seconds (-1 to disable)", type=int
    )

    parser.add_argument("--debug", help="Enable debug mode", action="store_true")

    parser.epilog = """To use multiple nodes, you can set the following environment variables:
- `SLURM_ARRAY_TASK_ID`: The index of the current node (starting at 1)
- `SLURM_ARRAY_TASK_MAX`: Number of nodes (or maximum index)
        """


def _main(args: argparse.Namespace):
    args_p = utils.load_config(args, DEFAULT_ARGS, handle=True)

    scheduler_path = Path(args_p["scheduler"])
    scheduler_found_path = shutil.which(scheduler_path.name, path=scheduler_path.parent)
    if scheduler_found_path is None:
        raise RuntimeError(
            f"Could not find the scheduler at {scheduler_path} or {scheduler_path.parent}"
        )
    scheduler = Path(scheduler_found_path).resolve()

    cp_solver: Path = args_p["cp_solver"].resolve()
    for mod_alg in args_p["modular_algorithm"]:
        if mod_alg != "constraint" and len(args_p["algorithm"]) == 0:
            raise ValueError(
                "At least one algorithm must be specified when using a modular algorithm"
            )
        if mod_alg == "constraint":
            cp_solver_found_path = shutil.which(cp_solver.name, path=cp_solver.parent)
            if cp_solver_found_path is None:
                raise RuntimeError(
                    f"Could not find the CP solver at {cp_solver} or {cp_solver.parent}"
                )
            cp_solver = Path(cp_solver_found_path).resolve()

    executor = Executor(
        input_dir=args_p["in"],
        output_dir=args_p["out"],
        scheduler=scheduler,
        cp_solver=cp_solver,
        runs=args_p["runs"],
        skip_exists=not args_p["run_all"],
        algorithms=args_p["algorithm"],
        mod_algorithms=args_p["modular_algorithm"],
        verbose=args_p["verbose"],
        time_out=args_p["time_out"],
    )
    executor.execute_tasks(
        debug=args_p["debug"],
        processes=args_p["processes"],
        make_dirs_only=args_p["make_dirs_only"],
    )


if __name__ == "__main__":
    m_parser = argparse.ArgumentParser()
    add_arguments(m_parser)
    m_args = m_parser.parse_args()
    try:
        _main(m_args)
    except KeyboardInterrupt:
        print("Stopping due to keyboard interrupt")
        print("Bye!")
        exit(0)
