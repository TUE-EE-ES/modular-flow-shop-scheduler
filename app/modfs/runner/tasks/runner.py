import json
import os
from collections.abc import Iterator
from dataclasses import dataclass
from pathlib import Path
from subprocess import STDOUT, run  # noqa: S404

from slurmer import Task, TaskParameters, TaskResult

from modfs.runner.input import FileId, find_input_files


@dataclass
class _ExecutorParameters(TaskParameters):
    scheduler: Path
    input_file: Path
    out_dir: Path
    file_id: FileId
    algorithm: str
    mod_algorithm: str
    verbose: bool
    run: int
    skip_exists: bool
    time_out: int

    def __hash__(self) -> int:
        return hash(self._get_tuple())

    def __lt__(self, other: "_ExecutorParameters") -> bool:
        return self._get_tuple() < other._get_tuple()

    def _get_tuple(self) -> tuple:
        return (
            self.scheduler,
            self.input_file,
            self.out_dir,
            self.file_id,
            self.algorithm,
            self.mod_algorithm,
            self.run,
        )


@dataclass
class _ExecutorResult(TaskResult):
    pass


class Executor(Task):
    """Represents all the tasks that need to be performed to solve the flow-shop problems."""

    def __init__(
        self,
        input_dir: Path,
        output_dir: Path,
        scheduler: Path,
        cp_solver: Path,
        runs: int,
        skip_exists: bool,
        algorithms: list[str],
        mod_algorithms: list[str],
        verbose: bool,
        time_out: int,
    ):
        super().__init__()
        self.input_dir: Path = input_dir.resolve()
        self.output_dir: Path = output_dir.resolve()
        self.scheduler: Path = scheduler
        self.cp_solver: Path = cp_solver
        self.runs: int = runs
        self.to_make_dirs: list[Path] = []
        self.to_write_jsons: list[tuple[Path, dict]] = []
        self.skip_exists: bool = skip_exists
        self.algorithms: list[str] = algorithms
        self.mod_algorithms: list[str] = mod_algorithms
        self.verbose: bool = verbose
        self.time_out: int = time_out

    def generate_parameters(self) -> Iterator[_ExecutorParameters]:
        """Generate all the different files that need to be created."""
        input_folders = find_input_files(self.input_dir)
        for json_path, json_info, input_files in input_folders:
            json_relative = json_path.relative_to(self.input_dir)
            json_info["original_path"] = str(os.path.relpath(json_path))
            json_info["runs"] = self.runs
            json_info["time_out"] = self.time_out

            for mod_algorithm in self.mod_algorithms:
                json_info["modular_algorithm"] = mod_algorithm

                if mod_algorithm == "constraint":
                    algorithms = [""]
                    scheduler = self.cp_solver
                else:
                    algorithms = self.algorithms
                    scheduler = self.scheduler

                for algorithm in algorithms:
                    json_info["algorithm"] = algorithm

                    for run_num in range(self.runs):
                        batch_out_dir = (
                            self.output_dir
                            / json_relative.parent
                            / mod_algorithm
                            / algorithm
                            / f"run_{run_num}"
                        )
                        batch_out_dir.mkdir(exist_ok=True, parents=True)
                        json_info["run"] = run_num
                        self.to_write_jsons.append(
                            (batch_out_dir / "files_info.json", json_info.copy())
                        )

                        for file in input_files:
                            out_dir = batch_out_dir / f"{file.file_id}"
                            self.to_make_dirs.append(out_dir)

                            yield _ExecutorParameters(
                                scheduler=scheduler,
                                input_file=file.path,
                                out_dir=out_dir,
                                file_id=file.file_id,
                                algorithm=algorithm,
                                mod_algorithm=mod_algorithm,
                                verbose=self.verbose,
                                run=run_num,
                                skip_exists=self.skip_exists,
                                time_out=self.time_out,
                            )

    def make_dirs(self):
        """Make the output directories."""
        for directory in self.to_make_dirs:
            directory.mkdir(exist_ok=True, parents=True)

        for json_path, json_data in self.to_write_jsons:
            if json_path.exists():
                continue
            with json_path.open("w") as f:
                json.dump(json_data, f, indent=4)

    @staticmethod
    def args_fms_scheduler(parameters: _ExecutorParameters) -> list[str | Path]:
        """Generate the arguments for the fms-scheduler."""
        return [
            parameters.scheduler,
            "--input",
            str(parameters.input_file),
            "--output",
            f"{parameters.file_id}",
            f"--algorithm={parameters.algorithm}",
            f"--modular-algorithm={parameters.mod_algorithm}",
            "--output-format=cbor",
            f"--time-out={parameters.time_out*1000}" if parameters.time_out > 0 else "",
            "-vvvvv" if parameters.verbose else "-vv",
        ]

    @staticmethod
    def args_constraint(parameters: _ExecutorParameters) -> list[str | Path]:
        """Generate the arguments for the constraint optimizer."""
        return [
            str(parameters.scheduler),
            str(parameters.input_file),
            str(parameters.out_dir / str(parameters.file_id)),
            f"--time-limit={parameters.time_out}" if parameters.time_out > 0 else "",
        ]

    @staticmethod
    def processor_function(parameters: _ExecutorParameters) -> _ExecutorResult:
        """Process a single file."""
        log_out_file = parameters.out_dir / f"{parameters.file_id}.out.log"

        if log_out_file.exists() and parameters.skip_exists:
            return _ExecutorResult()

        log_out_file_tmp = log_out_file.with_suffix(log_out_file.suffix + ".tmp")

        with open(log_out_file_tmp, "wb") as f:
            if parameters.mod_algorithm == "constraint":
                args = Executor.args_constraint(parameters)
            else:
                args = Executor.args_fms_scheduler(parameters)
            run(
                args,
                cwd=parameters.out_dir,
                stderr=STDOUT,
                stdout=f,
                check=True,
            )

        # Rename the file without the .tmp suffix to mark completion
        log_out_file_tmp.replace(log_out_file)
        return _ExecutorResult()
