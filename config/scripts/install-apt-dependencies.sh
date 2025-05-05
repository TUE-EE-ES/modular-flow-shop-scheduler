#!/bin/env bash
set -e

apt-get update
apt-get install -y \
    git wget graphviz libgraphviz-dev \
    python3-pip python3-venv pipx \
    ninja-build g++-14 gcc-14 openjdk-11-jre
apt-get clean
