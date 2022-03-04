ARG MANYLINUX_IMAGE=manylinux2014_x86_64

FROM quay.io/pypa/$MANYLINUX_IMAGE
LABEL authors="Pramod Kumbhar, Fernando Pereira, Alexandru Savulescu"

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
    autoconf \
    automake \
    openssh-server \
    libtool

WORKDIR /root

RUN wget http://ftpmirror.gnu.org/ncurses/ncurses-6.2.tar.gz \
    && tar -xvzf ncurses-6.2.tar.gz \
    && cd ncurses-6.2  \
    && ./configure --prefix=/nrnwheel/ncurses --without-shared CFLAGS="-fPIC" \
    && make -j install

RUN curl -L -o mpich-3.3.2.tar.gz http://www.mpich.org/static/downloads/3.3.2/mpich-3.3.2.tar.gz \
    && tar -xvzf mpich-3.3.2.tar.gz \
    && cd mpich-3.3.2 \
    && ./configure --prefix=/nrnwheel/mpich FFLAGS="-w -fallow-argument-mismatch -O2" \
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

ENV PATH /nrnwheel/openmpi/bin:$PATH
RUN yum -y install epel-release libX11-devel libXcomposite-devel vim-enhanced
RUN yum -y remove ncurses-devel

# Copy Dockerfile for reference
COPY Dockerfile .

# build wheels from there
WORKDIR /root
