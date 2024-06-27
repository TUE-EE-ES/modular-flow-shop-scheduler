# Must be run from the root directory of the project as such:
# docker build . -t modular-scheduling:single-node -f ./config/deploys/distributed-scheduling/single-node.Dockerfile

FROM ubuntu:22.04 as debug

ENV DEBIAN_FRONTEND=noninteractive PIP_ROOT_USER_ACTION=ignore

# Install dependencies of the scheduler
RUN <<EOF 
set -e
apt-get update
apt-get install -y \
    git python3-pip python3-venv wget graphviz libgraphviz-dev ninja-build software-properties-common
add-apt-repository -y ppa:ubuntu-toolchain-r/test
apt-get install -y g++-13 gcc-13 python-is-python3
apt-get clean
EOF
RUN wget -q -O cmake.sh https://github.com/Kitware/CMake/releases/download/v3.28.3/cmake-3.28.3-linux-x86_64.sh && \
    sh cmake.sh --skip-license --prefix=/usr/local

# Julia is used to generate the data for the experiments form the input specification files
RUN <<EOF
set -e
wget --progress=dot:giga https://julialang-s3.julialang.org/bin/linux/x64/1.10/julia-1.10.2-linux-x86_64.tar.gz
tar -xzf julia-1.10.2-linux-x86_64.tar.gz -C /opt
ln -s /opt/julia-1.10.2/bin/julia /usr/local/bin/julia
rm julia-1.10.2-linux-x86_64.tar.gz
EOF

# Poetry installs the dependencies of the python script used to run and analyze the results
RUN <<EOF
set -e
pip3 install pipx
pipx install poetry
ln -s /root/.local/bin/poetry /usr/local/bin/poetry
EOF

# Install cpoptimizer. The file must be located in the "extra" folder. This file is not provided as you require your 
# own license to download it from the IBM website.
COPY ./extra/cplex_studio2211.linux_x86_64.bin /
RUN <<EOF
set -e
/cplex_studio2211.linux_x86_64.bin -i silent -DLICENSE_ACCEPTED=TRUE -DUSER_INSTALL_DIR=/opt/ibm/ILOG/CPLEX_Studio2211
rm /cplex_studio2211.linux_x86_64.bin
ln -s /opt/ibm/ILOG/CPLEX_Studio2211/cpoptimizer/bin/x86-64_linux/cpoptimizer /usr/local/bin/cpoptimizer
EOF

COPY ./app /app
COPY ./config/cp-solver.sh /app/data/bin/cp-solver
WORKDIR /app

# Build the scheduler
RUN <<EOF
set -e
git config --global user.email "J.Marce.i.Igual@tue.nl"
git config --global user.name "J. Marcè i Igual"
git config --global init.defaultBranch main
git init
git add .
git commit -am "Initial commit"
cd scheduler/scheduler
CC=gcc-13 CXX=g++-13 cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
mkdir -p /app/data/bin
ln -s /app/scheduler/scheduler/build/bin/app /app/data/bin/scheduler
chmod +x /app/data/bin/cp-solver
EOF

# Generate the data for the experiments
RUN <<EOF
set -e
julia --project=. --threads=auto ./install.jl
julia --project=. --threads=auto ./app.jl gen --out data/gen/generic inputs/paper/distributed-scheduling
mkdir data/gen/computational
cp -R data/gen/generic/mixed/duplex/bookletA data/gen/generic/mixed/duplex/bookletAB data/gen/computational
EOF

# Install the python dependencies
RUN <<EOF
set -e
poetry config virtualenvs.in-project true
poetry install
cd models
poetry install --no-root
EOF

# RUN poetry run modfs run --modular-algorithm broadcast cocktail constraint --algorithm bhcs simple --time-limit 600 --in data/run/
# RUN poetry run modfs run --algorithm=bhcs --modular-algorithm broadcast cocktail constraint
# If you already have the data, you can comment the previous line and copy it to the container by uncommenting the following lines
COPY ./extra/data/run/ /app/data/run/

FROM ubuntu:22.04 as release

RUN cd /app/notebooks/papers/distributed-scheduling && poetry run jupyter execute distributed_scheduling.ipynb
