"""Module handling INFO files."""

import json
from collections.abc import Iterator
from pathlib import Path


MAX_DEPTH = 20


def load_info_files(path: Path) -> Iterator[tuple[Path, dict]]:
    """Load all info JSON files found in this directory and below.

    Args:
        path (Path): Path where to find the info files.

    Yields:
        Iterator[tuple[Path, dict]]: Iterator containing the path and the file content.
    """
    to_search: list[tuple[Path, int]] = [(path, 1)]

    while len(to_search) > 0:
        path, depth = to_search.pop()

        info_file = path / "files_info.json"

        if info_file.exists():
            with open(info_file, "r", encoding="utf-8") as file:
                json_info = json.load(file)

            yield info_file, json_info

        elif depth < MAX_DEPTH:
            for path_new in path.iterdir():
                if path_new.is_dir():
                    to_search.append((path_new, depth + 1))
