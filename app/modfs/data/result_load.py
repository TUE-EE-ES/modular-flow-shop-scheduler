"""Loads data from scheduling results."""

import os
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Iterator, NamedTuple, Optional, Literal, cast

import cbor2
import pandas as pd
from slurmer import Task, TaskParameters, TaskResult
from tqdm.auto import tqdm

from modfs.utils import load_info_files

_CACHE_FILE_NAME = "cache_v6.parquet"


class _ProblemInfo(NamedTuple):
    makespan: int
    iterations: Optional[int]
    total_time: int
    time_per_job: int
    timeout: bool
    solved: bool
    error: str
    optimal: bool
    optimality_gap: float
    lower_bound: int


@dataclass(slots=True)
class _LoaderParameters(TaskParameters):
    idx: int
    cbor_file: Path

    def __hash__(self) -> int:
        return hash(self.idx)


@dataclass(slots=True)
class _LoaderResult(TaskResult):
    idx: int
    problem_info: _ProblemInfo


class _Loader(Task):
    def __init__(
        self,
        cbor_files: list[Path],
    ):
        super().__init__()
        self.cbor_files = cbor_files
        self.problem_info: list[_LoaderResult] = []

    def generate_parameters(self) -> Iterator[_LoaderParameters]:
        for i, file in enumerate(self.cbor_files):
            yield _LoaderParameters(idx=i, cbor_file=file)

    @staticmethod
    def processor_function(parameters: TaskParameters) -> Optional[_LoaderResult]:
        parameters = cast(_LoaderParameters, parameters)

        with open(parameters.cbor_file, mode="rb") as f:
            cbor_data: dict = cbor2.load(f)  # type: ignore # VS Code complains and it is wrong

            return _LoaderResult(
                parameters.idx,
                _ProblemInfo(
                    makespan=cbor_data.get("minMakespan", None),  # May be missing if not solved
                    iterations=cbor_data.get("iterations", None),
                    total_time=cbor_data.get("totalTime", None),
                    time_per_job=cbor_data.get("timePerJob", None),
                    timeout=cbor_data.get("timeout", None),
                    solved=cbor_data["solved"],
                    error=cbor_data.get("error", ""),
                    optimal=cbor_data.get("optimal", False),
                    optimality_gap=cbor_data.get("optimalityGap", None),
                    lower_bound=cbor_data.get("lowerBound", None),
                ),
            )

    def process_output(self, result: TaskResult) -> bool:
        result = cast(_LoaderResult, result)
        self.problem_info.append(result)
        return False


def _get_cached_or_to_load(path: Path) -> tuple[list[Path], list[pd.DataFrame], pd.DataFrame]:
    print("Looking for files to load")

    data = defaultdict(list)
    cbor_files = []
    df_cached = []
    info_files = list(load_info_files(path))
    for info_file_path, json_info in tqdm(info_files, desc="Loading info files"):
        cache_file = info_file_path.parent / _CACHE_FILE_NAME
        if cache_file.exists():
            df_cached.append(pd.read_parquet(cache_file))
            continue

        json_info["original_path"] = json_info["original_path"].replace("/files_info.json", "")
        num_files = len(json_info["files"])

        data["run_file_path"] += [str(info_file_path.parent)] * num_files
        data["original_path"] += [json_info["original_path"]] * num_files

        data["run"] += [json_info["runs"]] * num_files
        data["algorithm"] += [json_info["algorithm"]] * num_files
        data["modular_algorithm"] += [json_info["modular_algorithm"]] * num_files
        data["time_limit"] += [json_info["time_out"]] * num_files

        for file_id, file_info in json_info["files"].items():
            folder_path = info_file_path.parent / file_id

            data["file_id"].append(int(file_id))
            data["jobs"].append(file_info["jobs"])
            data["modules"].append(file_info["modules"])

            # If the file does not exist, skip it and show a warning
            cbor_file = folder_path / f"{file_id}.cbor"
            cbor_file_new = folder_path / f"{file_id}.fms.cbor"
            if not cbor_file.exists() and not cbor_file_new.exists():
                print(f"WARNING: {cbor_file} or {cbor_file_new} does not exist")
                continue
            if cbor_file_new.exists():
                cbor_file = cbor_file_new
            cbor_files.append(cbor_file)

    return cbor_files, df_cached, pd.DataFrame.from_dict(data)


def _write_cache(df_loaded: pd.DataFrame):
    for info_file_path, df in df_loaded.groupby("run_file_path"):
        cache_file = Path(str(info_file_path)) / _CACHE_FILE_NAME
        df.to_parquet(cache_file, index=False)


def load_results(path: Path) -> pd.DataFrame:
    cbor_files, df_cached, df_info_files = _get_cached_or_to_load(path)

    if len(cbor_files) > 0:
        # We need to load new files
        loader = _Loader(cbor_files)
        loader.execute_tasks(description="Loading cbor files", debug=False)
        loaded_results = map(
            lambda x: x.problem_info, sorted(loader.problem_info, key=lambda x: x.idx)
        )
        df_loaded_results = pd.DataFrame.from_records(loaded_results, columns=_ProblemInfo._fields)
        df_loaded = pd.concat([df_info_files, df_loaded_results], axis=1)
        _write_cache(df_loaded)

        df_cached.append(df_loaded)

    print("All files have been loaded!")
    return pd.concat(df_cached, axis=0, ignore_index=True)

def add_derived_columns(
    df: pd.DataFrame,
    exclude_paths: list[str] | None = None,
    groups: dict[str, set[str]] | None = None,
    gen_subpath: str = "",
    run_subpath: Path | None = None,
    group_source: Literal["original_path", "run_file_path"] = "original_path",
    extra_main_keys: Optional[list[str]] = None,
) -> pd.DataFrame:
    """Generate derived columns from a result data frame.

    Args:
        df: Result data frame as loaded by load_results.
        exclude_paths (optional): List of paths to remove from the DataFrame. It
            uses the `original_path` column. If None, no path is removed. Defaults to None.
        groups (optional): Groups to use for the `group` column. The
            key of the dictionary will be used as the group name and the rows whose `original_path`
            column matches any of the values of the dictionary will be added to the given group.
            If None, a column with an empty "" group will be created. Defaults to None.
        gen_subpath (optional): Subpath to remove from the `original_path` column.
        run_subpath (optional): Subpath to remove from the `run_file_path` column.
        group_source (optional): Column to use for grouping. Defaults to "original_path".

    Returns:
        pandas.DataFrame: Original dataframe with the derived columns added.
    """
    if exclude_paths is None:
        exclude_paths = []

    df = df.loc[~df["original_path"].isin(exclude_paths)].copy()
    df["time_per_iteration"] = df["total_time"] / df["iterations"]
    df["time_per_job"] = df["total_time"] / df["jobs"]
    df["original_path"] = df["original_path"].str.replace(rf"{gen_subpath}/(.*)", r"\1", regex=True)
    df["run_file_path"] = df["run_file_path"].str.replace(r"run_\d+", "", regex=True)

    if run_subpath is not None:

        def _remove_path_prefix(path_str: str) -> str:
            try:
                return os.path.relpath(path_str, run_subpath).replace("\\", "/")
            except ValueError:
                return path_str

        df["run_file_path"] = df["run_file_path"].map(_remove_path_prefix)

    df["group"] = ""
    if groups is not None:
        for group, values in groups.items():
            selected = df[group_source].str.startswith(tuple(values))
            df.loc[selected, "group"] = group

    # We drop run_file_path because there's the option of running experiments multiple times
    # and the run_file_path will be different for each run. That is why we assign the groups
    # before dropping the column.
    main_keys = (
        [
            "group",
            "run_file_path",
            "original_path",
            "file_id",
            "modules",
            "jobs",
            "modular_algorithm",
            "algorithm",
            "time_limit",
        ]
        + []
        if extra_main_keys is None
        else extra_main_keys
    )
    df = df.groupby(main_keys, as_index=False, dropna=False).agg(
        {
            "iterations": "mean",
            "total_time": "mean",
            "time_per_job": "mean",
            "makespan": "mean",
            "optimality_gap": "min",
            "lower_bound": "max",
            "timeout": "all",
            "solved": "any",
            "optimal": "any",
            "error": "first",
        }
    )
    df.sort_values(main_keys, inplace=True)

    # If error is empty and solved is True, set it to 'none'
    df.loc[df.eval("error == '' and solved"), "error"] = "none"  # type: ignore

    # set group as a new level of the multi-level index
    return df.astype({k: "category" for k in ["run_file_path", "original_path"]})
