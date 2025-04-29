#!/bin/env bash

set -e

# Install Julia packages
julia --project=. --threads=auto ./install.jl

# Generate data
julia --project=. --threads=auto ./app.jl gen --out data/gen/generic inputs/paper/modular-scheduling

# The computational data is just a subset of the original data
mkdir data/gen/computational
cp -R data/gen/generic/mixed/duplex/bookletA data/gen/generic/mixed/duplex/bookletAB data/gen/computational

