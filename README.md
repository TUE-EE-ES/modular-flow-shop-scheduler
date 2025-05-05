# Modular scheduling experiments

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.15306150.svg)](https://doi.org/10.5281/zenodo.15306150) [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

This repository contains all the code to generate, run and analyse the experiments for the paper
"Modular Scheduling of Tightly Coupled Production Lines".

## Table of contents

- [Modular scheduling experiments](#modular-scheduling-experiments)
  - [Table of contents](#table-of-contents)
  - [Project structure](#project-structure)
  - [Docker implementation (recommended)](#docker-implementation-recommended)
    - [Requisites](#requisites)
    - [Single-node docker image](#single-node-docker-image)
    - [Multi-node-enabled docker image](#multi-node-enabled-docker-image)
      - [Generic experiments](#generic-experiments)
      - [Computational experiments](#computational-experiments)
      - [Experiments reusing schedulers](#experiments-reusing-schedulers)
      - [Analysis of the results](#analysis-of-the-results)
  - [Local implementation](#local-implementation)
    - [Requirements](#requirements)
    - [Generating input problems](#generating-input-problems)
    - [Compile MAS implementation](#compile-mas-implementation)
    - [Install CP python dependencies (optional)](#install-cp-python-dependencies-optional)
    - [Running the experiments](#running-the-experiments)
    - [Analysing the results](#analysing-the-results)

## Project structure

You can use Docker to run the experiments (see #docker-implementation-recommended) or run them locally (see #local-implementation). The project is structured as follows.

- `app/`: Contains the code to generate, run and analyse the experiments.
- `config/`: Contains the configuration files to build the docker images.
- `extra/`: Contains the extra files needed to build the docker images.

Inside the `app/` directory you can find the following:

- `scheduler/`: Contains the MAS implementation in C++ as well as the local schedulers implementations.
- `app.jl`: The main julia script to generate the input problems. You can find the input problems in `inputs/`. Run `julia --project=. ./app.jl gen --help` to see the available options.
- `modfs/`: Contains the python package to run the experiments. You can run the experiments with `poetry run modfs run --help`.
- `notebooks/papers/modular-scheduling/paper.ipynb`: Contains the jupyter notebook to analyse the results.
- `models/`: Contains the CP model to solve the production line problem.

## Docker implementation (recommended)

For convenience, we've provided two docker images that can be used to recreate the experiments:

- The single-node docker image: creates a docker image that installs the dependencies, runs and
  analyses the experiments in a single node. However, because the analysis is not parallelized
  it can take a long time to execute.
- The multi-node-enabled docker: image creates a docker image with the dependencies to run the
  experiments in a multi-node environment. The image is built without running the experiments and
  it is expected to be run in a multi-node environment.

### Requisites

To build the docker images you need the following:

- Docker installed in your machine. You can download it from the [official website](https://www.docker.com/get-started).
- The CP Optimizer installation file (`cplex_studio2212.linux_x86_64.bin`). You can download the file from the IBM website. You need to create an account to download the file and have a valid license. Place the file in the `extra` directory: `extra/cplex_studio2212.linux_x86_64.bin`.

### Single-node docker image

For convenience, we've provided a Dockerfile that installs the dependencies, runs and analyses the
experiments. To build the docker image do the following:

```bash
docker build . -t modular-scheduling:single-node -f ./config/deploys/modular-scheduling/single-node.Dockerfile
```

> **Caution**: The docker image is quite large (~30GB) and it will take a long time to run the
> experiments.

### Multi-node-enabled docker image

We've also provided a docker image that is capable of running the experiments in a multi-node
environment. This image is built without running the experiments and it is expected to be run in a
multi-node environment. To build the docker image run the following:

```bash
docker build . -t modular-scheduling:multi-node -f ./config/deploys/modular-scheduling/multi-node.Dockerfile
```

To run the experiments you need to pass the `SLURM_ARRAY_TASK_ID` and `SLURM_ARRAY_TASK_MAX` as
environment variables. You don't need SLURM to use them, they are simply read by the python script. `SLURM_ARRAY_TASK_ID` starts at 1 and ends at `SLURM_ARRAY_TASK_MAX`. You also need to mount the `/app/data/run/` directory.

There are three types of experiments, the _generic_, the _computational_ and the _reusing schedulers_ experiments. The generic are run with a time limit of 600 seconds, the computational both with 600 and 3600 seconds for the MAS and only 3600 seconds for the CP and the reusing with a time limit of 3600 seconds.

#### Generic experiments

```bash
# Needs to run in a single node. Creates the folder structure used by all nodes
docker run -v <path_to_shared_data_folder>/run:/app/data/run/ modular-scheduling:multi-node \
  poetry run modfs run --make-dirs-only --algorithm bhcs simple --modular-algorithm broadcast cocktail constraint \
  --time-limit 600 --in /app/data/gen/generic --out /app/data/run/generic

# Run in all nodes. Starts the analysis in parallel in all nodes. 
# If you don't have slurm, just make sure that the environment variables are set correctly.
docker run -v <path_to_shared_data_folder>/run:/app/data/run/ \
  -e SLURM_ARRAY_TASK_MAX=<number_of_nodes> \
  -e SLURM_ARRAY_TASK_ID=<id_of_node> modular-scheduling:multi-node\
  poetry run modfs run --algorithm bhcs simple --modular-algorithm broadcast cocktail constraint \
  --time-limit 600 --in /app/data/gen/generic --out /app/data/run/generic
```

#### Computational experiments

```bash
# Needs to run in a single node. Creates the folder structure used by all nodes
docker run -v <path_to_shared_data_folder>/run:/app/data/run/ modular-scheduling:multi-node \
  poetry run modfs run --make-dirs-only --algorithm bhcs simple --modular-algorithm broadcast cocktail \
  --time-limit 600 --in /app/data/gen/computational --out /app/data/run/computational
docker run -v <path_to_shared_data_folder>/run:/app/data/run/ modular-scheduling:multi-node \
  poetry run modfs run --make-dirs-only --algorithm bhcs simple --modular-algorithm broadcast cocktail constraint \
  --time-limit 3600 --in /app/data/gen/computational --out /app/data/run/computational


# Run in all nodes. Starts the analysis in parallel in all nodes. 
# If you don't have slurm, just make sure that the environment variables are set correctly.
docker run -v <path_to_shared_data_folder>/run:/app/data/run/ \
  -e SLURM_ARRAY_TASK_MAX=<number_of_nodes> \
  -e SLURM_ARRAY_TASK_ID=<id_of_node> modular-scheduling:multi-node\
  poetry run modfs run --algorithm bhcs --modular-algorithm broadcast cocktail \
  --time-limit 600 --in /app/data/gen/computational --out /app/data/run/computational

# Run analysis of the computational experiments with 3600 seconds time limit. This can be run in 
# parallel/different nodes with the previous command.
docker run -v <path_to_shared_data_folder>/run:/app/data/run/ \
  -e SLURM_ARRAY_TASK_MAX=<number_of_nodes> \
  -e SLURM_ARRAY_TASK_ID=<id_of_node> modular-scheduling:multi-node\
  poetry run modfs run --algorithm bhcs simple --modular-algorithm broadcast cocktail constraint \
  --time-limit 3600 --in /app/data/gen/computational --out /app/data/run/computational
  
```

#### Experiments reusing schedulers

The experiments reusing schedulers reuse the same input files as the ones for the computational experiments. We only change the scheduler used by each module.

```bash
# Needs to run in a single node. Creates the folder structure used by all nodes
docker run -v <path_to_shared_data_folder>/run:/app/data/run/ modular-scheduling:multi-node \
  poetry run modfs run --make-dirs-only \
  --algorithm mneh-asap-backtrack bhcs-mneh-asap-backtrack \
  --modular-algorithm broadcast cocktail \
  --time-limit 3600 --in /app/data/gen/computational --out /app/data/run/computational


# Run in all nodes. Starts the analysis in parallel in all nodes. 
# If you don't have slurm, just make sure that the environment variables are set correctly.
docker run -v <path_to_shared_data_folder>/run:/app/data/run/ modular-scheduling:multi-node \
  poetry run modfs run \
  -e SLURM_ARRAY_TASK_MAX=<number_of_nodes> \
  -e SLURM_ARRAY_TASK_ID=<id_of_node> modular-scheduling:multi-node\
  --algorithm mneh-asap-backtrack bhcs-mneh-asap-backtrack \
  --modular-algorithm broadcast cocktail \
  --time-limit 3600 --in /app/data/gen/computational --out /app/data/run/computational
```

#### Analysis of the results

After the experiments have run, you can analyse the results and generate the figures of the paper
by running the following:

```bash
docker run \
  -v <path_to_shared_data_folder>/run:/app/data/run/ \
  -v <path_to_shared_data_folder>/figs:/app/data/figs/ \
  -w /app/notebooks/papers/modular-scheduling modular-scheduling:multi-node \
  poetry run jupyter execute paper.ipynb
```

## Local implementation

You can also run the experiments locally without requiring docker to do so, you'll need to:

1. Install all the requirements
2. Generate the input problems
3. Compile the MAS implementation
4. Install
5. Run the experiments
6. Analyse the results

In the following steps we use `/` to indicate the project root directory.

### Requirements

- Julia 1.10.2 or higher
- CMake 3.21 or higher
- A C++ compiler that supports C++20 (e.g. GCC 13 or higher)
- Python 3.10 or higher
- Poetry 1.1.8 or higher
- CP Optimizer by IBM version (we used version 22.1.1.0)

### Generating input problems

The input problems are generated from the YAML files located in  `/app/inputs/paper` using julia.
To generate them you need julia in your path, go to the `/app` folder and run the following:

```bash
julia --project=. --threads=auto ./install.jl # Install Julia dependencies
julia --project=. --threads=auto ./app.jl gen --out data/gen/generic inputs/paper/distributed-scheduling
```

This will generate a folder `/app/data/gen/generic` with the input problems. To generate the computational experiments you need to copy the subset of the generic problems to the computational folder:

```bash
mkdir data/gen/computational
cp -R data/gen/generic/mixed/duplex/bookletA data/gen/generic/mixed/duplex/bookletAB data/gen/computational
```

### Compile MAS implementation

The MAS implementation with the BHCS and simple schedulers is located in the `/app/scheduler/scheduler` folder. The code is written in C++ and it is compiled using CMake. To compile the code go to the `/app/scheduler/scheduler` directory and run the following:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Link the binary to the `/app/data/bin/scheduler` directory:

```bash
ln -s build/bin/app ../../data/bin/scheduler
```

### Install CP python dependencies (optional)

If you want to run the CP model you need to install the python dependencies. To do so go to the `/app/models` directory and run the following:

```bash
# Make sure to have virtualenvs.in-project set to true because data/bin/cp-solver requires it
poetry config virtualenvs.in-project true
poetry install --no-root
```

### Running the experiments

The experiments are run in parallel by a python package called `modfs`
installed by poetry in its python environment. To install the python environment go to the
`/app` directory and run the following:

```bash
poetry install
```

To run the experiments you need the scheduler and the CP optimizer by IBM. The scheduler is
located in the `app/scheduler` directory. After building the scheduler, the python script expects
to find the scheduler in `app/data/bin/scheduler`. You can create a symlink. The CP optimizer is
expected to be in the `PATH`. However, the python script expects to find a file
`app/data/bin/cp-solver` that runs the python script in `app/models/src/cpModelProductionLine.py`.
We've provided an example file in `config/cp-solver.sh` that you can use as a template. For
most use cases just renaming it to `cp-solver` and making it executable should be enough.

To run the experiments you need to run the following:

```bash
# Generic experiments
poetry run modfs run --algorithm bhcs simple --modular-algorithm broadcast cocktail constraint --time-out 600 --in data/gen/generic --out data/run/generic

# Computational experiments
poetry run modfs run --algorithm bhcs simple --modular-algorithm broadcast cocktail --time-out 600 --in data/gen/computational --out data/run/computational
poetry run modfs run --algorithm bhcs simple --modular-algorithm broadcast cocktail constraint --time-out 3600 --in data/gen/computational --out data/run/computational

# Experiments reusing schedulers
poetry run modfs run --algorithm mneh-asap-backtrack bhcs-mneh-asap-backtrack --modular-algorithm broadcast cocktail --time-out 3600 --in data/gen/computational --out data/run/computational
```

### Analysing the results

The results are analysed using a python jupyter notebook. The
jupyter dependencies are already installed together with the `modfs` package. To analyse the
results open the jupyter notebook `app/notebooks/papers/paper.ipynb`. Make sure
that the working directory is the same as the file.

The figures from the paper are generated in `/app/data/figs/paper/modular-scheduling`.
