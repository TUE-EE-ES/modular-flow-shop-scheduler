import argparse
import os
import json
import re
import subprocess
from pathlib import Path


PROG = re.compile(r"#\s*yaml-language-server:\s*\$schema=(.*)")
ROOT = Path(__file__).parent.parent


def find_schema_file(file: Path, filejson: Path) -> Path | None:
    with open(file) as f:
        # Read line by line
        for line in f:
            m = PROG.match(line)
            if not m:
                continue

            schemafilestr = m.group(1)
            schemafile = (file.parent / Path(schemafilestr)).resolve()
            if schemafile.exists():
                return schemafile
            break

    # We couldn't find it in the comment. Try the JSON file
    with open(filejson) as f:
        data = json.load(f)

    if "$schema" not in data:
        return None

    schemafile = (filejson.parent / Path(data["$schema"])).resolve()
    if not schemafile.exists():
        return None

    return schemafile


def test_file(file: Path):
    # Convert the YAML to JSON using yq
    filejson = file.with_suffix(".json")
    os.system(f"yq -o=j {file} > {filejson}")

    schemafile = find_schema_file(file, filejson)
    if schemafile is None:
        print(f"Could not find schema file for {file}")
        filejson.unlink()
        return

    # Validate the JSON file with the schema
    out = subprocess.run(
        ["ajv", "--spec=draft2020", "-s", str(schemafile), "-d", str(filejson)],
        capture_output=True,
        shell=True,
    )
    filejson.unlink()

    if out.returncode != 0:
        print(f"Validation failed for {file}")
        print(out.stdout.decode("utf-8"))
        print(out.stderr.decode("utf-8"))


def main(args: dict):
    for path in args["paths"]:
        path: Path = Path(path).absolute()
        print(path)

        if path.is_file():
            test_file(path)
            continue
        for file in path.glob("**/*.yaml"):
            test_file(file)


if __name__ == "__main__":
    parser = argparse.ArgumentParser("Input files validator")
    parser.add_argument(
        "paths", nargs="*", help="Paths to validate", default=[(ROOT / "inputs").absolute()]
    )

    args = parser.parse_args()
    main(vars(args))
