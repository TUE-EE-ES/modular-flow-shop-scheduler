# Must be run from the root directory of the project as such:
# docker build . -t modular-scheduling:single-node -f ./config/deploys/modular-scheduling/single-node.Dockerfile

FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive PIP_ROOT_USER_ACTION=ignore

COPY ./config/scripts /opt/scripts

# Install dependencies of the scheduler
RUN /opt/scripts/install-apt-dependencies.sh
RUN wget -q -O cmake.sh https://github.com/Kitware/CMake/releases/download/v3.28.3/cmake-3.28.3-linux-x86_64.sh && \
    sh cmake.sh --skip-license --prefix=/usr/local

# Julia is used to generate the data for the experiments form the input specification files
RUN /opt/scripts/install-julia.sh

# Poetry installs the dependencies of the python script used to run and analyze the results
RUN /opt/scripts/install-poetry.sh

# Install cpoptimizer. The file must be located in the "extra" folder. This file is not provided as you require your 
# own license to download it from the IBM website.
COPY ./extra/cplex_studio2211.linux_x86_64.bin /
RUN /opt/scripts/install-cplex.sh

COPY ./app /app
WORKDIR /app

# Build the scheduler
RUN /opt/scripts/build-scheduler.sh

# Generate the data for the experiments
RUN /opt/scripts/generate-data.sh

# Install the python dependencies
RUN /opt/scripts/install-python-dependencies.sh

# Run the generic experiments
RUN poetry run modfs run --modular-algorithm broadcast cocktail constraint --algorithm bhcs simple --time-out 600 --in data/gen/generic --out data/run/generic

# Run the computational experiments with 600 seconds time limit. Constraint is not required here
RUN poetry run modfs run --modular-algorithm broadcast cocktail --algorithm bhcs simple --time-out 600 --in data/gen/computation --out data/run/computational

# Run the computational experiments with 3600 seconds time limit
RUN poetry run modfs run --modular-algorithm broadcast cocktail constraint --algorithm bhcs simple --time-out 3600 --in data/gen/computation --out data/run/computational

# Run the reusing schedulers experiments
RUN poetry run modfs run --modular-algorithm broadcast cocktail --algorithm mneh-asap-backtrack bhcs-mneh-asap-backtrack --time-out 3600 --in data/gen/computational --out data/run/computational

# If you already have the data, you can comment the previous lines and copy it to the container by uncommenting the following lines
# COPY ./extra/data/run/ /app/data/run/

RUN cd /app/notebooks/papers/modular-scheduling && poetry run jupyter execute paper.ipynb
