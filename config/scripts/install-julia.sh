#!/bin/env bash

set -e

# Download and extract
wget --progress=dot:giga https://julialang-s3.julialang.org/bin/linux/x64/1.10/julia-1.10.2-linux-x86_64.tar.gz
tar -xzf julia-1.10.2-linux-x86_64.tar.gz -C /opt

# Link
ln -s /opt/julia-1.10.2/bin/julia /usr/local/bin/julia

# Clean up
rm julia-1.10.2-linux-x86_64.tar.gz

