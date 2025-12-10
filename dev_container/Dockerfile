FROM docker.io/fedora:42

# NOTE: Fedora 42 comes with GCC 15 by default, which is not supported by CUDA
# 12.9 or below, so we install GCC 14 instead.
ENV CUDA_SUPPORTED_GCC_PACKAGE=gcc14-c++
RUN dnf -y install \
    sudo \
    git \
    wget \
    make \
    cmake \
    ninja \
    vim \
    curl \
    unzip \
    flex \
    mpich-devel \
    openmpi-devel \
    bison \
    autoconf \
    automake \
    patchelf \
    openssh-server \
    python-devel \
    libdwarf-devel \
    elfutils-devel \
    libunwind-devel \
    binutils-devel \
    ncurses-devel \
    readline-devel \
    libXcomposite-devel \
    libXext-devel \
    ccache \
    gcc-c++ \
    ${CUDA_SUPPORTED_GCC_PACKAGE} \
    gdb \
    valgrind \
    rr \
    clang \
    libcxx-devel \
    lcov \
    procps \
    doxygen \
    ffmpeg \
    pandoc \
    xorg-x11-server-Xvfb \
    awk \
    libtool && yum -y clean all && rm -rf /var/cache

# we select the OpenMPI implementation as the default, but any other one would
# work as well
ENV PATH="/usr/lib64/openmpi/bin:$PATH"

# caching both Python (uv) packages and ccache
ENV UV_CACHE_DIR=/opt/cache/python
ENV CCACHE_DIR=/opt/cache/ccache
ENV CMAKE_GENERATOR=Ninja
ENV CMAKE_C_COMPILER_LAUNCHER=ccache
ENV CMAKE_CXX_COMPILER_LAUNCHER=ccache
# TODO: change this every time Python is updated
ENV NMODL_PYLIB=/usr/lib64/libpython3.13.so.1.0

# setup venv
RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"

WORKDIR /scratch

# install MUSIC
RUN python -m pip install 'uv==0.9.15'
RUN python -m uv pip install 'mpi4py' 'cython' 'setuptools'

ENV MUSIC_VERSION=13f312338dcccebfe74d391b1b24f1b6d816ac6c
RUN curl -L -o MUSIC.zip https://github.com/INCF/MUSIC/archive/${MUSIC_VERSION}.zip
RUN unzip MUSIC.zip && mv MUSIC-* MUSIC
WORKDIR /scratch/MUSIC
RUN ./autogen.sh
RUN ./configure --with-python-sys-prefix --disable-anysource
RUN make -j install
WORKDIR /scratch

# install caliper
ENV CALIPER_VERSION=v2.12.1
ENV CALIPER_SOURCE=https://github.com/LLNL/Caliper.git
RUN git clone --branch ${CALIPER_VERSION} --depth 1 --recurse-submodules --shallow-submodules ${CALIPER_SOURCE} caliper
RUN cmake -B caliper_build -G Ninja caliper
RUN sudo cmake --build caliper_build --target install --parallel

# install likwid
# NOTE: the default configuration of likwid does not support NVHPC
ENV LIKWID_VERSION=v5.5.0
ENV LIKWID_SOURCE=https://github.com/RRZE-HPC/likwid.git
RUN git clone --branch ${LIKWID_VERSION} --depth 1 --recurse-submodules --shallow-submodules ${LIKWID_SOURCE} likwid
WORKDIR /scratch/likwid
# likwid does not autodetect the architecture, so we need to manually specify the compiler
RUN if [ "$(uname -m)" = "aarch64" ]; then sed -i 's/^COMPILER = .*/COMPILER = GCCARMv8/' config.mk; fi
RUN make -j
RUN make install
WORKDIR /scratch

# by default, MPI complains about running as root; as we are in a container
# already, we can safely run it
ENV OMPI_ALLOW_RUN_AS_ROOT=1
ENV OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1


WORKDIR /nrn

# add automatic NVHPC detection
RUN echo 'for dir in /opt/nvidia/hpc_sdk/Linux_*/*/compilers/bin; do \
  if [ -d "${dir}" ]; then \
    export PATH="${dir}:${PATH}"; \
    export CMAKE_CUDA_COMPILER_LAUNCHER=ccache; \
    break; \
  fi \
done' >> /etc/bashrc
