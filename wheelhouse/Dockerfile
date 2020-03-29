FROM quay.io/pypa/manylinux2010_x86_64
LABEL authors="Pramod Kumbhar,Fernando Pereira"

# Need to install sudo first:
RUN yum install -y sudo
# Can only run sudo from a terminal on CentOS 5.

# While we are still root, install the necessary dependencies for Malmo:
RUN yum -y install \
    git \
    swig \
    doxygen \
    libxslt \
    gcc-c++ \
    bzip2-devel \
    python34-tkinter \
    wget \
    software-properties-common \
    xpra \
	libgl1-mesa-dri \
    make \
    vim \
    curl \
    gfortran \
    unzip \
    bison \
    flex \
    pkg-config \
    autoconf \
    automake \
    make \
    openssh-server \
    libopenmpi-dev \
    libhdf5-serial-dev \
    libtool \
    zlib-devel \
    cmake3 \
    ncurses-devel.x86_64 \
    gcc \
    patch \
    make

RUN sudo yum -y install cmake3 ncurses-devel.x86_64 gcc patch make
ENV PATH ${HOME}/.local/bin:$PATH

ENV PATH /opt/python/cp27-cp27m/bin:/opt/python/cp27-cp27mu/bin:/opt/python/cp35-cp35m/bin:/opt/python/cp36-cp36m/bin:/opt/python/cp37-cp37m/bin:$PATH

# default software required
RUN  python2.7 -m pip install --upgrade --user scipy matplotlib bokeh ipython cython pytest
RUN  python3.5 -m pip install --upgrade --user scipy matplotlib bokeh ipython cython pytest
RUN  python3.6 -m pip install --upgrade --user scipy matplotlib bokeh ipython cython pytest
RUN  python3.7 -m pip install --upgrade --user scipy matplotlib bokeh ipython cython pytest

# default software required
RUN  python2.7 -m pip install --upgrade --user twine pip
RUN  python3.5 -m pip install --upgrade --user twine pip
RUN  python3.6 -m pip install --upgrade --user twine pip
RUN  python3.7 -m pip install --upgrade --user twine pip

RUN  python3.5 -m pip install --upgrade --user git+https://github.com/ferdonline/auditwheel@fix/rpath_append
RUN  python3.6 -m pip install --upgrade --user git+https://github.com/ferdonline/auditwheel@fix/rpath_append
RUN  python3.7 -m pip install --upgrade --user git+https://github.com/ferdonline/auditwheel@fix/rpath_append

# sometime get git clone error: RPC failed; curl 56
RUN git clone --depth=1 https://github.com/neuronsimulator/nrn.git -b python_wheel neuron-yale

WORKDIR ${HOME}/neuron-yale
RUN python2.7 setup.py bdist_wheel
RUN python3.5 setup.py bdist_wheel
RUN python3.6 setup.py bdist_wheel
RUN python3.7 setup.py bdist_wheel

# repair wheel? see : https://malramsay.com/post/perils_of_packaging/
RUN  python3.5 -m auditwheel repair dist/NEURON-7.8.11-cp35-cp35m-linux_x86_64.whl
RUN  python3.6 -m auditwheel repair dist/NEURON-7.8.11-cp36-cp36m-linux_x86_64.whl
RUN  python3.7 -m auditwheel repair dist/NEURON-7.8.11-cp37-cp37m-linux_x86_64.whl

# auditwheel doesn't support python < 3.5 and have seen issues with repaired wheel from python3
RUN cp dist/NEURON-7.8.11-cp27-cp27m-linux_x86_64.whl wheelhouse/
RUN python3.5 -m auditwheel repair dist/NEURON-7.8.11-cp27-cp27m-linux_x86_64.whl
