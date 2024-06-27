"""Plotting utilities."""

from enum import Enum, auto
from typing import cast, Optional
from collections.abc import Callable

import pandas as pd
import plotly.express as px
from plotly.basedatatypes import BaseFigure


DEFAULT_LABELS = {
    "algorithm": "Scheduling algorithm",
    "iterations": "Number of iterations",
    "jobs": "Number of jobs",
    "makespan": "Makespan",
    "relative_makespan": "Relative Makespan",
    "modular_algorithm": "Algorithm",
    "modules": "# modules",
    "time_per_iteration": "Execution time (ms) per iteration",
    "time_per_job": "Execution time (ms) per job",
    "timeout": "Timeout",
    "total_time": "Run time (ms)",
    "group": "Benchmark",
    "backtrack": "Monolithic",
    "cocktail": "Modular Cocktail",
    "broadcast": "Modular Broadcast",
    "time_limit": "Time Limit (s)",
    "optimality_gap": "Optimality Gap",
    "optimality_ratio": "Optimality Ratio",
}


def plot_line_defaults(**kwargs) -> BaseFigure:
    """Make a line plot with some defaults already set. You can override them by passing them as
    keyword arguments.

    Returns:
        px.Figure: Generated figure
    """
    # We use the dictionary so we can update the default values
    return px.line(
        **{
            "facet_row": "algorithm",
            "facet_col": "modular_algorithm",
            "markers": False,
            "color": "modules",
            "line_dash": "timeout",
            **kwargs,
            "labels": {
                **DEFAULT_LABELS,
                **kwargs.get("labels", {}),
            },
        }
    )


def line_labels(data: Optional[pd.DataFrame | pd.Series] = None, **kwargs) -> BaseFigure:
    """Plot a line graph with the default labels."""
    if data is not None:
        kwargs["data_frame"] = data

    return px.line(
        **{
            **kwargs,
            "labels": {
                **DEFAULT_LABELS,
                **kwargs.get("labels", {}),
            },
        }
    )


class PlotType(Enum):
    """Type of plot to use. Used by some functions"""

    BOX = auto()
    BAR = auto()


def _plot_boxy_defaults(func: Callable = px.box, **kwargs) -> BaseFigure:
    return func(
        **{
            "x": "group",
            "color": "modular_algorithm",
            **kwargs,
            "labels": {
                **DEFAULT_LABELS,
                **kwargs.get("labels", {}),
            },
        }
    )


def plot_boxy(
    data: Optional[pd.DataFrame | pd.Series] = None, plot_type: PlotType = PlotType.BAR, **kwargs
) -> BaseFigure:
    """Plot a box graph with the default labels."""
    if data is not None:
        kwargs["data_frame"] = data

    match plot_type:
        case PlotType.BAR:
            return _plot_boxy_defaults(func=px.bar, **kwargs)
        case PlotType.BOX:
            return _plot_boxy_defaults(func=px.box, **kwargs)

    return _plot_boxy_defaults(**kwargs)


def add_timeout(df: pd.DataFrame) -> pd.DataFrame:
    """Adds a column `timeout` to the dataframe that is True if an instance timed out."""
    df = df.copy()
    df_time = df.groupby(["original_path", "file_id"], group_keys=False)["timeout"]
    df["timeout_file"] = df_time.apply(lambda x: x | x.any())
    return df


def add_all_solved(df: pd.DataFrame) -> pd.DataFrame:
    """Adds a column `all_solved` to the dataframe that is True if an instance was solved by all
    algorithms."""

    s_tmp = df.groupby(["group", "original_path", "file_id", "modular_algorithm"])["solved"].any()
    s_tmp = s_tmp.groupby(["group", "original_path", "file_id"]).all()
    s_tmp.name = "all_solved"
    return df.merge(s_tmp, on=["group", "original_path", "file_id"])


def baseline_extract(
    df: pd.DataFrame, group_name: str, group_column: str = "modular_algorithm"
) -> tuple[pd.DataFrame, pd.DataFrame]:
    """Extracts a dataframe to be used as a baseline for comparison.

    It will use the `modular_algorithm` column by default.

    Args:
        df (pd.DataFrame): Original dataframe
        group_name (str): Group to be used as a baseline
        group_column (str): Column where to find the baseline group

    Returns:
        tuple[pd.DataFrame, pd.DataFrame]: Two dataframes, the first one is the original dataframe
            without the baseline and the second is the baseline dataframe.
    """
    df_baseline_bool = df[group_column] == group_name
    df_baseline = df[df_baseline_bool]
    df_other = cast(pd.DataFrame, df[~df_baseline_bool])
    return df_other, df_baseline


def baseline_compare(
    df: pd.DataFrame,
    group_name: str,
    value_column: str = "makespan",
    group_column: str = "modular_algorithm",
):
    """Compare the value of `value_column` between all groups and the selected baseline.

    Args:
        df (pd.DataFrame): DataFrame with the data
        group_name (str): Name of the group to use as baseline.
        value_column (str, optional): Column name of the value to compare. Defaults to "makespan".
        group_column (str, optional): Column name of the group to compare. Defaults to
            "modular_algorithm".

    Returns:
        pd.DataFrame: DataFrame containing the ratio of the value of `value_column` between the
            the other groups and the baseline :math:`\\frac{value_other}{value_baseline}`.
    """
    return baseline_generic_compare(*baseline_extract(df, group_name, group_column), value_column)


def baseline_generic_compare(
    df: pd.DataFrame, df_baseline: pd.DataFrame, column: str
) -> pd.DataFrame:
    group_keys = ["group", "original_path", "file_id"]
    df_tmp_base = df_baseline[group_keys + [column]]
    df_compared = df.merge(df_tmp_base, on=group_keys, suffixes=("", "_baseline"))
    df_compared[column] /= df_compared[f"{column}_baseline"]
    return df_compared


def baseline_makespan_compare(df: pd.DataFrame, df_baseline: pd.DataFrame) -> pd.DataFrame:
    """Generate a dataframe with the makespan ratio between `df` and `df_baseline`.

    Args:
        df (pd.DataFrame): Dataframe that will be compared
        df_baseline (pd.DataFrame): Dataframe that will be used as baseline

    Returns:
        pd.DataFrame: Dataframe where the `makespan` column is the makespan ratio. It
            contains the same rows as `df` but the order may be altered.
    """
    return baseline_generic_compare(df, df_baseline, "makespan")


def baseline_runtime_compare(df: pd.DataFrame, df_baseline: pd.DataFrame) -> pd.DataFrame:
    return baseline_generic_compare(df, df_baseline, "total_time")


def create_by_jobs_df(df: pd.DataFrame) -> pd.DataFrame:
    return cast(
        pd.DataFrame,
        df.groupby(
            ["algorithm", "modular_algorithm", "modules", "timeout", "jobs"],
            as_index=False,
        )["makespan"].mean(),
    )


def makespan_by_jobs(
    df: pd.DataFrame, name: str = "", title: str | None = None, create_df: bool = True, **kwargs
) -> BaseFigure:
    if title is None:
        title = f"Makespan depending on number of jobs and modules of {name}"
    return plot_line_defaults(
        data_frame=create_by_jobs_df(df) if create_df else df,
        x="jobs",
        y="makespan",
        title=title,
        **kwargs,
    )


def makespan_baseline_by_jobs(df: pd.DataFrame, df_baseline: pd.DataFrame, name: str) -> BaseFigure:
    fig = plot_line_defaults(
        data_frame=create_by_jobs_df(baseline_makespan_compare(df, df_baseline)),
        x="jobs",
        y="makespan",
        title=f"Makespan compared to backtrack (lower is better) of {name}<br>"
        "Depending on number of jobs and modules",
    )
    fig.add_hline(y=1, line={"color": "red"}, annotation_text="Baseline")
    return fig


def makespan_all(
    df: pd.DataFrame,
    plot_type: PlotType = PlotType.BAR,
    title: str = "Makespan of all the problems by benchmark",
    subtitle: str = "",
    **kwargs,
) -> BaseFigure:
    match plot_type:
        case PlotType.BAR:
            func = px.bar
            df_processed = df.groupby(
                [
                    "group",
                    "modular_algorithm",
                    "algorithm",
                ],
                as_index=False,
                sort=False,
            )["makespan"].mean()
            extra_settings = {
                "barmode": "group",
            }
        case PlotType.BOX:
            func = px.box
            df_processed = df
            extra_settings = {
                "points": False,
            }
        case _:
            raise NotImplementedError(f"Plot type {plot_type} not implemented")

    return _plot_boxy_defaults(
        func,
        data_frame=df_processed,
        y="makespan",
        title=f"{title}{subtitle}",
        **extra_settings,
        **kwargs,
    )


def makespan_all_baseline(
    df: pd.DataFrame,
    baseline_algorithm: str,
    title="Makespan ratio of all the problems by benchmark",
    **kwargs,
) -> BaseFigure:
    df_compared = baseline_makespan_compare(*baseline_extract(df, baseline_algorithm))

    fig = makespan_all(
        df_compared,
        title=title,
        subtitle="<br><sup>Lower is better</sup>",
        plot_type=PlotType.BOX,
        **kwargs,
    )
    fig.add_hline(y=1, line={"color": "red"}, annotation_text="Baseline")
    print(df_compared.groupby(["modular_algorithm"])["makespan"].mean().to_string())
    print(df_compared.groupby(["group", "modular_algorithm"])["makespan"].mean().to_string())
    return fig


def runtime_by_jobs(
    df: pd.DataFrame, name: str = "", title: str | None = None, **kwargs
) -> BaseFigure:
    if title is None:
        title = f"Execution time depending on number of jobs and modules of {name}"
    return plot_line_defaults(
        data_frame=df,
        x="jobs",
        y="total_time",
        title=title,
        **kwargs,
    )


def runtime_all(
    df: pd.DataFrame,
    plot_type: PlotType = PlotType.BAR,
    title: str = "Execution time of all the problems by benchmark",
    subtitle: str = "",
) -> BaseFigure:
    match plot_type:
        case PlotType.BAR:
            func = px.bar
            df_processed = df.groupby(
                [
                    "group",
                    "modular_algorithm",
                    "algorithm",
                ],
                as_index=False,
                sort=False,
            )["total_time"].mean()
            extra_settings = {
                "barmode": "group",
            }
        case PlotType.BOX:
            func = px.box
            df_processed = df
            extra_settings = {
                "points": False,
                "log_y": True,
            }
        case _:
            raise NotImplementedError(f"Plot type {plot_type} not implemented")

    return _plot_boxy_defaults(
        func,
        data_frame=df_processed,
        y="total_time",
        title=f"{title}{subtitle}",
        **extra_settings,
    )


def runtime_all_box(df: pd.DataFrame) -> BaseFigure:
    return runtime_all(df, PlotType.BOX)


def runtime_all_baseline(
    df: pd.DataFrame,
    baseline_algorithm: str,
    plot_type: PlotType = PlotType.BAR,
) -> BaseFigure:
    df_compared = baseline_runtime_compare(*baseline_extract(df, baseline_algorithm))

    fig = runtime_all(df_compared, plot_type, subtitle="<br><sup>Higher is better</sup>")
    fig.add_hline(y=1, line={"color": "red"}, annotation_text="Baseline")
    return fig


def runtime_all_baseline_box(df: pd.DataFrame, baseline_algorithm: str) -> BaseFigure:
    return runtime_all_baseline(df, baseline_algorithm, PlotType.BOX)


def runtime_baseline_by_jobs(df: pd.DataFrame, df_baseline: pd.DataFrame, name: str) -> BaseFigure:
    fig = plot_line_defaults(
        data_frame=create_by_jobs_df(baseline_runtime_compare(df, df_baseline)),
        x="jobs",
        y="total_time",
        log_y=True,
        title=f"Execution time compared to backtrack (lower is better) of {name}<br>"
        "Depending on number of jobs and modules",
    )
    fig.add_hline(y=1, line={"color": "red"}, annotation_text="Baseline")
    return fig


def iterations_by_jobs(df: pd.DataFrame, name: str) -> BaseFigure:
    return plot_line_defaults(
        data_frame=df,
        x="jobs",
        y="iterations",
        title=f"Number of iterations depending on number of jobs and modules of {name}",
    )


def time_per_iteration_by_jobs(df: pd.DataFrame, name: str) -> BaseFigure:
    return plot_line_defaults(
        data_frame=df,
        x="jobs",
        y="time_per_iteration",
        title=f"Execution time per iteration depending on number of jobs and modules of {name}",
    )


def time_per_job_by_jobs(df: pd.DataFrame, name: str) -> BaseFigure:
    return plot_line_defaults(
        data_frame=df,
        x="jobs",
        y="time_per_job",
        title=f"Execution time per job depending on number of jobs and modules of {name}",
    )
