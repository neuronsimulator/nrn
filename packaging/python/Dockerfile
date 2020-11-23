FROM quay.io/pypa/manylinux1_x86_64
LABEL authors="Pramod Kumbhar,Fernando Pereira"

# install basic packages
RUN yum -y install \
    git \
    wget \
    make \
    vim \
    curl \
    unzip \
    bison \
    flex \
    pkg-config \
    autoconf \
    automake \
    make \
    openssh-server \
    libtool

WORKDIR /root

RUN curl -L -o cmake-3.12.3.tar.gz https://github.com/Kitware/CMake/releases/download/v3.12.3/cmake-3.12.3.tar.gz \
    && tar -xvzf cmake-3.12.3.tar.gz \
    && cd cmake-3.12.3 \
    && ./bootstrap -- -DCMAKE_INSTALL_PREFIX=/nrnwheel/cmake \
    && make -j install

RUN wget http://ftpmirror.gnu.org/ncurses/ncurses-5.9.tar.gz \
    && tar -xvzf ncurses-5.9.tar.gz \
    && cd ncurses-5.9  \
    && ./configure --prefix=/nrnwheel/ncurses --without-shared CFLAGS="-fPIC" \
    && make -j install

RUN curl -L -o mpich-3.3.2.tar.gz http://www.mpich.org/static/downloads/3.3.2/mpich-3.3.2.tar.gz \
    && tar -xvzf mpich-3.3.2.tar.gz \
    && cd mpich-3.3.2 \
    && ./configure --prefix=/nrnwheel/mpich \
    && make -j install

RUN curl -L -o openmpi-4.0.3.tar.gz  https://download.open-mpi.org/release/open-mpi/v4.0/openmpi-4.0.3.tar.gz \
    && tar -xvzf openmpi-4.0.3.tar.gz \
    && cd openmpi-4.0.3 \
    && ./configure --prefix=/nrnwheel/openmpi \
    && make -j install

RUN curl -L -o readline-7.0.tar.gz https://ftp.gnu.org/gnu/readline/readline-7.0.tar.gz \
    && tar -xvzf readline-7.0.tar.gz \
    && cd readline-7.0  \
    && ./configure --prefix=/nrnwheel/readline --disable-shared CFLAGS="-fPIC" \
    && make -j install

# create readline with ncurses
RUN cd /nrnwheel/readline/lib \
    && ar -x libreadline.a \
    && ar -x ../../ncurses/lib/libncurses.a \
    && ar cq libreadline.a *.o \
    && rm *.o

ENV PATH /nrnwheel/openmpi/bin:/nrnwheel/cmake/bin:$PATH

RUN yum -y install libX11-devel.x86_64 libXcomposite-devel.x86_64 vim-enhanced.x86_64
RUN yum -y remove ncurses-devel.x86_64

# Copy Dockerfile for reference
COPY Dockerfile .

# build wheels from there
WORKDIR /root
