# Modular scheduling experiments

This repository contains all the code to generate, run and analyse the experiments for the paper
"Modular Scheduling of Tightly Coupled Production Lines". 

## Project structure

The code is structured as follows:

- *Generation of input problems*: The input problems are generated from the YAML files located in
  `app/inputs/paper` using julia. To generate them you need julia in your path and run the 
  following:

  ```bash
  julia --project=. --threads=auto ./install.jl
  julia --project=. --threads=auto ./app.jl gen inputs/paper/distributed-scheduling
  ```
- *Running the experiments*: The experiments are run in parallel by a python package called `modfs`
  installed by poetry in its python environment. To install the python environment go to the 
  `app` directory and run the following:

  ```bash
  poetry install
  ```

  To run the experiments you need the scheduler and the CP optimizer by IBM. The scheduler is 
  located in the `app/scheduler` directory. After building the scheduler, the python script expects
  to find the scheduler in `app/data/bin/scheduler`. You can create a symlink. The CP optimizer is
  expected to be in the `PATH`. However, the python script expects to find a file 
  `app/data/bin/cp-solver` that runs the python script in `app/models/src/cpModelProductionLine.py`.
  We've provided an example file `app/data/bin/cp-solver.sh` that you can use as a template. For
  most use cases just renaming it to `cp-solver` and making it executable should be enough.
  
  To run the experiments you need to run the following:

  ```bash
  poetry run modfs run --algorithm=bhcs --modular-algorithm broadcast cocktail constraint
  ```
- *Analysis of the results*: The results are analysed using a python jupyter notebook. The 
  jupyter dependencies are already installed together with the `modfs` package. To analyse the 
  results open the jupyter notebook `app/notebooks/papers/distributed-scheduling.ipynb`. Make sure
  that the working directory is the same as the file.


## Docker implementation

For convenience, we've provided a Dockerfile that installs the dependencies, runs and analyses the
experiments. To build the docker image run the following:

```bash
docker build . -t modular-scheduling:single-node -f ./config/deploys/distributed-scheduling/single-node.Dockerfile
```

> **Caution**: The docker image is quite large (~30GB) and it will take a long time to run the
> experiments.

### Multi-node-enabled docker image

We've also provided a docker image that is capable of running the experiments in a multi-node
environment. This image is built without running the experiments and it is expected to be run in a
multi-node environment. To build the docker image run the following:

```bash
docker build . -t modular-scheduling:multi-node -f ./config/deploys/distributed-scheduling/multi-node.Dockerfile
```

To run the experiments you need to pass the `SLURM_ARRAY_TASK_ID` and `SLURM_ARRAY_TASK_MAX` as
environment variables. You also need to mount the `/app/data/run/` directory. To run the experiments
you need to run the following:

```bash
# Needs to run in a single node. Creates the folder structure used by all nodes
docker run -v <path_to_shared_data_folder>/run:/app/data/run/ modular-scheduling:multi-node poetry run modfs run --algorithm=bhcs --modular-algorithm broadcast cocktail constraint --dry-run
```

After the experiments have run, you can analyse the results and generate the figures of the paper
by running the following:

```bash
docker run -w /app/notebooks/papers/distributed_scheduling.ipynb -v <path_to_shared_data_folder>/run:/app/data/run/ -v <path_to_shared_data_folder>/figs:/app/data/figs/ modular-scheduling:multi-node poetry run jupyter execute
```
