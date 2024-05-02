# Modular scheduling

This repository contains all the scripts needed to evaluate the modular scheduling solutions through the `modfs` command. You can use:

- `modfs gen` to generate re-entrant flow-shop problems.
- `modfs run` to find solutions for the generated problems.

The options are as follows:

### Generator (`app.jl gen`)
```
usage: app.jl gen [--yaml YAML] [--print-defaults] [-o OUT] [-h]
                  [spec]

positional arguments:
  spec              YAML file containing the flow-shop specification
                    or folder containing YAML files.

optional arguments:
  --yaml YAML       YAML file (with comment support) containing
                    arguments. Can be used instead of passing the
                    optional command line arguments. The command line
                    arguments always have preference.
  --print-defaults  Shows the default values used by the arguments
  -o, --out OUT     Path where the output files should be saved,
                    directories will be created if they do not exist
  -h, --help        show this help message and exit
```                        

### Runner (`modfs run`)

```
usage: modfs run [-h] [--print-defaults] [--out OUT] [--in IN] [--runs RUNS] [--verbose] [--skip-exists] [--processes PROCESSES] [--scheduler SCHEDULER]
                 [--algorithm {bhcs,mdbhcs,backtrack} [{bhcs,mdbhcs,backtrack} ...]] [--modular-algorithm {all,cocktail,backtrack} [{all,cocktail,backtrack} ...]] [--debug]

options:
  -h, --help            show this help message and exit
  --print-defaults      Shows the default values used by the arguments
  --out OUT, -o OUT     Path to directory where to output all the files (will be created if it does not exist)
  --in IN, -i IN        Path to directory where to find the files to be processed
  --runs RUNS           Number of times each problem will be evaluated
  --verbose             Show verbose information when running algorithm
  --skip-exists         Skip result if it already exists
  --processes PROCESSES
                        Number of parallel processes to use when running
  --scheduler SCHEDULER
                        Path to the FMS-scheduler executable usually called app. It is recommended to create a symlink to the default location
  --algorithm {bhcs,mdbhcs,backtrack} [{bhcs,mdbhcs,backtrack} ...]
                        Algorithm used by the scheduler
  --modular-algorithm {all,cocktail,backtrack} [{all,cocktail,backtrack} ...]
                        Algorithm used by the modular constraint propagation
  --debug               Enable debug mode
  ```
For multiple nodes you can specify the following environment variables:

- `SLURM_ARRAY_TASK_ID`: The id of the current node starting at 1
- `SLURM_ARRAY_TASK_MAX`: The total number of nodes
