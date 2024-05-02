from collections.abc import Iterator
from dataclasses import dataclass, field
from pathlib import Path
from typing import NewType

from modfs.utils import load_info_files

FileId = NewType("FileId", int)


@dataclass
class SchedulerInputFile:
    """Representation of a single flow-shop XML file location and information."""

    file_id: FileId = FileId(-1)
    path: Path = field(default_factory=Path)
    extra_info: dict = field(default_factory=dict)


def find_input_files(path: Path) -> Iterator[tuple[Path, dict, list[SchedulerInputFile]]]:
    """Find the input XML files that need to be processed."""
    for info_file_path, json_info in load_info_files(path):
        result = []
        for file_id, file_info in json_info["files"].items():
            file_path = info_file_path.parent / f"{file_id}.xml"

            if not file_path.exists():
                print(f'WARNING: File "{file_path}" is declared in JSON info but cannot be found!')
                continue

            result.append(
                SchedulerInputFile(
                    file_id=FileId(file_id),
                    path=file_path.resolve(),
                    extra_info=file_info,
                )
            )
        yield info_file_path, json_info, result
