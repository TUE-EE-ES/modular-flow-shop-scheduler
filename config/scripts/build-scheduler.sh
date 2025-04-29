#!/bin/env bash

set -e

# The scheduler CMake script assumes it's inside a repository. Create a dummy one.
git config --global user.email "j.marce.i.igual@tue.nl"
git config --global user.name "J. Marcè i Igual"
git config --global init.defaultBranch main
git init
git add .
git commit -am "Initial commit"

# Start building
cd scheduler/scheduler
CC=gcc-13 CXX=g++-13 cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Deploy the binary
mkdir -p /app/data/bin
ln -s /app/scheduler/scheduler/build/bin/fms-scheduler /app/data/bin/scheduler

# Also add the symlink to the cp-solver binary. This is used by the scripts that run all the experiments
mv /app/data/bin/cp-solver.default.sh /app/data/bin/cp-solver
chmod +x /app/data/bin/cp-solver

