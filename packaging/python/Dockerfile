ARG MANYLINUX_IMAGE=manylinux2014_x86_64

FROM quay.io/pypa/$MANYLINUX_IMAGE
LABEL authors="Pramod Kumbhar, Fernando Pereira, Alexandru Savulescu"

RUN gcc --version && python --version

# install basic packages
RUN yum -y install \
    git \
    wget \
    make \
    vim \
    curl \
    unzip \
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

# newer flex with rpmbuild (manylinux2014 based on Centos7 currently has flex < 2.6)
RUN rpmbuild --rebuild https://vault.centos.org/8-stream/AppStream/Source/SPackages/flex-2.6.1-9.el8.src.rpm \
    && yum -y install rpmbuild/RPMS/*/flex-2.6.1-9.el7.*.rpm \
    && rm -rf rpmbuild

RUN wget http://ftpmirror.gnu.org/ncurses/ncurses-6.4.tar.gz \
    && tar -xvzf ncurses-6.4.tar.gz \
    && cd ncurses-6.4  \
    && ./configure --prefix=/nrnwheel/ncurses --without-shared --without-debug CFLAGS="-fPIC" \
    && make -j install \
    && cd .. && rm -rf ncurses-6.4 ncurses-6.4.tar.gz

RUN curl -L -o mpich-3.3.2.tar.gz http://www.mpich.org/static/downloads/3.3.2/mpich-3.3.2.tar.gz \
    && tar -xvzf mpich-3.3.2.tar.gz \
    && cd mpich-3.3.2 \
    && ./configure --disable-fortran --prefix=/nrnwheel/mpich \
    && make -j install \
    && cd .. && rm -rf mpich-3.3.2 mpich-3.3.2.tar.gz \
    && rm -rf /nrnwheel/mpich/share/doc /nrnwheel/mpich/share/man

RUN curl -L -o openmpi-4.0.3.tar.gz  https://download.open-mpi.org/release/open-mpi/v4.0/openmpi-4.0.3.tar.gz \
    && tar -xvzf openmpi-4.0.3.tar.gz \
    && cd openmpi-4.0.3 \
    && ./configure --prefix=/nrnwheel/openmpi \
    && make -j install \
    && cd .. && rm -rf openmpi-4.0.3 openmpi-4.0.3.tar.gz

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

ENV PATH /nrnwheel/openmpi/bin:$PATH
RUN yum -y install epel-release libX11-devel libXcomposite-devel vim-enhanced && yum -y clean all && rm -rf /var/cache
RUN yum -y remove ncurses-devel

# Copy Dockerfile for reference
COPY Dockerfile .

# build wheels from there
WORKDIR /root

ENV NMODL_PYLIB=/nrnwheel/python/lib/libpython3.10.so.1.0
