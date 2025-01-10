ARG MANYLINUX_IMAGE=manylinux_2_28_x86_64

FROM quay.io/pypa/$MANYLINUX_IMAGE
LABEL authors="Pramod Kumbhar, Fernando Pereira, Alexandru Savulescu, Goran Jelic-Cizmek"

RUN gcc --version && python3 --version

# install basic packages
RUN yum -y install \
    git \
    wget \
    make \
    vim \
    curl \
    unzip \
    flex \
    readline \
    ncurses \
    mpich \
    openmpi \
    bison \
    autoconf \
    automake \
    openssh-server \
    libtool && yum -y clean all && rm -rf /var/cache
    
# required for rpmbuild
RUN yum -y install \
    gettext \
    gcc-c++ \
    help2man \
    rpm-build && yum -y clean all && rm -rf /var/cache

WORKDIR /root

# NOTE: NMODL transpiler is used in during build step but it requires
# sympy+python available in order to translate MOD files to C++. But under
# manylinux container, Python (libpython) is not available and hence we can't
# complete the build step. In order to enable building wheels with the existing
# implementation of NMODL, for now, we are making libpython available inside
# the manylinux container which is just used during the build step (nothing is
# linked to libraries or binaries of distribution).
RUN curl -L -o Python-3.10.0.tar.gz https://www.python.org/ftp/python/3.10.0/Python-3.10.0.tgz \
    && tar -xvzf Python-3.10.0.tar.gz \
    && cd Python-3.10.0 \
    && ./configure --prefix=/nrnwheel/python --enable-shared --with-static-libpython=no \
    && make -j altinstall \
    && cd .. && rm -rf Python-3.10.0 Python-3.10.0.tar.gz

RUN yum -y install epel-release libX11-devel libXcomposite-devel vim-enhanced && yum -y clean all && rm -rf /var/cache

# Copy Dockerfile for reference
COPY Dockerfile .

# build wheels from there
WORKDIR /root

# remove Python 3.13t since we do not support the free-threaded build yet
RUN rm -fr /opt/python/cp313-cp313t

ENV NMODL_PYLIB=/nrnwheel/python/lib/libpython3.10.so.1.0
