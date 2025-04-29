#!/usr/bin/env bash

set -e

# Place here the absolute path to the repository
PATH_MPILP_REPO=""

cd "$PATH_MPILP_REPO"
source .venv/bin/activate
python3 src/cpModelProductionLine.py "$@"
