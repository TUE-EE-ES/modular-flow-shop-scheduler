#!/usr/bin/env bash

set -eou pipefail
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
DATA_DIR="$(realpath "$SCRIPT_DIR/../data")"

export RESTIC_REPOSITORY="$DATA_DIR/backup/restic"
export RESTIC_PASSWORD="sam-fms"

cd "$DATA_DIR"
restic backup --host co29 --pack-size 50 --exclude "cache*.parquet" --compression max "run" "gen"
