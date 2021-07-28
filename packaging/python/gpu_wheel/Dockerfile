FROM neuronsimulator/neuron_wheel
# FROM manylinux_nvhpc
LABEL authors="Pramod Kumbhar, Fernando Pereira, Alexandru Savulescu"

WORKDIR /root

COPY install_nvhpc.sh .
RUN bash install_nvhpc.sh

RUN yum install -y envi ronment-modules \
    && echo "module use /opt/nvidia/hpc_sdk/modulefiles" >> ~/.bashrc
    && /opt/nvidia/hpc_sdk/Linux_x86_64/21.7/compilers/bin/makelocalrc -x \
        -gcc `which gcc` -gpp `which g++` -g77 `which gfortran` -cuda 11.0
