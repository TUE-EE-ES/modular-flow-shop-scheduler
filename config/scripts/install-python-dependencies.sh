#!/bin/env bash

set -e

poetry config virtualenvs.in-project true
poetry install

# Install the CP models. Not as root because the project is not defined as a package but just as a bunch
# of scripts with their dependencies.
cd models
poetry install --no-root