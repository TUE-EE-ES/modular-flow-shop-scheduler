#!/bin/env bash

set -e

/cplex_studio2211.linux_x86_64.bin -i silent -DLICENSE_ACCEPTED=TRUE -DUSER_INSTALL_DIR=/opt/ibm/ILOG/CPLEX_Studio2211
rm /cplex_studio2211.linux_x86_64.bin
ln -s /opt/ibm/ILOG/CPLEX_Studio2211/cpoptimizer/bin/x86-64_linux/cpoptimizer /usr/local/bin/cpoptimizer
