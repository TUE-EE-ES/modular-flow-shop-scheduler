FROM ubuntu:22.04

RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y \
    git python3-pip wget graphviz ninja-build qt6-base-dev libgl1-mesa-dev libglx-dev doxygen software-properties-common
RUN add-apt-repository -y ppa:ubuntu-toolchain-r/test
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y g++-13 gcc-13
RUN apt-get clean
RUN wget -q -O cmake.sh https://github.com/Kitware/CMake/releases/download/v3.28.3/cmake-3.28.3-linux-x86_64.sh && \
    sh cmake.sh --skip-license --prefix=/usr/local
RUN pip3 install gcovr
