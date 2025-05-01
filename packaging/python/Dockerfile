ARG MANYLINUX_IMAGE=manylinux_2_28_x86_64

FROM quay.io/pypa/$MANYLINUX_IMAGE
LABEL authors="Pramod Kumbhar, Fernando Pereira, Alexandru Savulescu, Goran Jelic-Cizmek"

# problem: libstdc++ is _not_ forwards compatible, so if we try to compile mod
# files on a system that ships a version of it older than the one used for
# building the wheel itself, we'll get linker errors.
# solution: use a well-defined oldest-supported version of GCC
# we need to do this _before_ building any libraries from source
ARG OLDEST_SUPPORTED_GCC_VERSION=10
RUN yum -y install \
    gcc-toolset-${OLDEST_SUPPORTED_GCC_VERSION}-gcc \
    gcc-toolset-${OLDEST_SUPPORTED_GCC_VERSION}-gcc-c++ \
    && yum -y clean all && rm -rf /var/cache
ENV PATH /opt/rh/gcc-toolset-${OLDEST_SUPPORTED_GCC_VERSION}/root/usr/bin:$PATH
ENV LD_LIBRARY_PATH=/opt/rh/gcc-toolset-${OLDEST_SUPPORTED_GCC_VERSION}/root/usr/lib64:/opt/rh/gcc-toolset-${OLDEST_SUPPORTED_GCC_VERSION}/root/usr/lib:/opt/rh/gcc-toolset-${OLDEST_SUPPORTED_GCC_VERSION}/root/usr/lib64/dyninst:/opt/rh/gcc-toolset-${OLDEST_SUPPORTED_GCC_VERSION}/root/usr/lib/dyninst
ENV DEVTOOLSET_ROOTPATH=/opt/rh/gcc-toolset-${OLDEST_SUPPORTED_GCC_VERSION}/root

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
    mpich-devel \
    openmpi-devel \
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

RUN wget http://ftpmirror.gnu.org/ncurses/ncurses-6.4.tar.gz \
    && tar -xvzf ncurses-6.4.tar.gz \
    && cd ncurses-6.4  \
    && ./configure --prefix=/nrnwheel/ncurses --without-shared --without-debug CFLAGS="-fPIC" \
    && make -j install \
    && cd .. && rm -rf ncurses-6.4 ncurses-6.4.tar.gz

RUN curl -L -o readline-7.0.tar.gz https://ftp.gnu.org/gnu/readline/readline-7.0.tar.gz \
    && tar -xvzf readline-7.0.tar.gz \
    && cd readline-7.0  \
    && ./configure --prefix=/nrnwheel/readline --disable-shared CFLAGS="-fPIC" \
    && make -j install \
    && cd .. && rm -rf readline-7.0 readline-7.0.tar.gz

# create readline with ncurses
RUN cd /nrnwheel/readline/lib \
    && ar -x libreadline.a \
    && ar -x ../../ncurses/lib/libncurses.a \
    && ar cq libreadline.a *.o \
    && rm *.o

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
RUN yum -y remove ncurses-devel

# build wheels from there
WORKDIR /root

# remove Python 3.13t since we do not support the free-threaded build yet
RUN rm -fr /opt/python/cp313-cp313t

ENV NMODL_PYLIB=/nrnwheel/python/lib/libpython3.10.so.1.0

ENV PATH /usr/lib64/openmpi/bin:$PATH

# Copy Dockerfile for reference
COPY Dockerfile .
