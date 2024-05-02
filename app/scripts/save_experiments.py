#!/usr/bin/env python3
import subprocess
import datetime
import argparse
import shutil
import os
import sys
from pathlib import Path

PATH_ROOT = Path(__file__).parent.parent.resolve()
PATH_DATA = PATH_ROOT / "data"


def main(args: dict):
    path_7zip = shutil.which("7zz")
    if path_7zip is None:
        raise FileNotFoundError("7zz not found. Is it installed?")
    path_7zip = Path(path_7zip)

    name_out = args["name"]
    if len(name_out) == 0:
        # Use the current timestamp
        name_out = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S.7z")
    path_out = PATH_DATA / "backup" / name_out
    path_out.parent.mkdir(parents=True, exist_ok=True)

    path_cwd = os.getcwd()
    os.chdir(PATH_DATA)

    cli_args = [
        str(path_7zip),
        "a",
        "-mx=9",
        # "-mqs=on",
        "-myx=9",
        str(path_out.relative_to(PATH_DATA)),
        "run",
        "gen",
    ]
    print(" ".join(f"'{arg}'" for arg in cli_args))
    subprocess.run(cli_args, stdout=sys.stdout, stderr=sys.stderr, check=True)
    os.chdir(path_cwd)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Save experiments from the out folder")

    parser.add_argument("--name", type=str, default="", help="Name of the saved file")

    main(vars(parser.parse_args()))
