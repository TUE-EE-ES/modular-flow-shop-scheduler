# Flexible Manufacturing System Scheduler

## Table of Contents
- [Flexible Manufacturing System Scheduler](#flexible-manufacturing-system-scheduler)
  - [How to Install](#how-to-install)
  - [Building the scheduler](#building-the-scheduler)
  - [Building the visualization](#building-the-visualization)

- Links:
  - [Documentation](https://sam-fms.pages.tue.nl/fms-scheduler)
  - [Coverage](coverage/index.html)

## How to Install

Install Git, if you don't have it already

For the core scheduler code, you can use GCC (MingW), LLVM etc. For the graph visualization, you will need Qt:
Download Qt from <https://www.qt.io/download-open-source>
Download Qt 5.15.1 (with for example MingW 8.1.0 if you are working on Windows)

Clone fms-scheduler sources:
<https://gitlab.tue.nl/sam-fms/fms-scheduler>

Execute the following from a command prompt which has git on the path:

```sh
git clone git@gitlab.tue.nl:sam-fms/fms-scheduler.git
cd fms-scheduler
```

## Building the scheduler

To build and run the scheduler, execute the following commands:

```sh
cd scheduler
cmake -S . -B build
cmake --build build
```

**NOTE**: On Windows you should use a Visual Studio developer console

## Building the visualization

To build and run the flow shop visualization, execute the following commands:

```sh
cd scheduler
cmake -S . -B build -DBUILD_TOOLS=ON
cmake --build build
```

**NOTE**: On Windows you should use a Visual Studio developer console and have the CMAKE_PREFIX_PATH environment variable set to the Qt 5.15.1 MSVC installation directory

## Scripts

Download Python 3 (Anaconda distribution, or do `pip install pandas matplotlib intervaltree`) for the prerequisites to run the `verify.py` and `experiment` scripts.


