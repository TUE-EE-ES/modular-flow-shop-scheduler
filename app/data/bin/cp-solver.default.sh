#!/usr/bin/env bash

set -e

cd "/app/models"
source .venv/bin/activate
python3 src/cpModelProductionLine.py "$@"
