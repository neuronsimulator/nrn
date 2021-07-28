FROM neuronsimulator/neuron_wheel
# FROM manylinux_nvhpc
LABEL authors="Pramod Kumbhar, Fernando Pereira, Alexandru Savulescu"

WORKDIR /root

COPY install_nvhpc.sh .

RUN bash install_nvhpc.sh
RUN yum install -y environment-modules \
    && echo "module use /opt/nvidia/hpc_sdk/modulefiles" >> ~/.bashrc
