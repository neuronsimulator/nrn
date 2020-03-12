# NEURON-CoreNEURON Benchmark 

###### tags: `NEURON` `CoreNEURON` `GPU` `HPC` `Benchmark`

### Introduction

NEURON is a simulation environment developed over past three decades for modeling networks of neurons with complex branched anatomy and biophysical membrane properties. This includes extracellular potential near membranes, multiple channel types, inhomogeneous channel distribution and ionic accumulation. It can handle diffusion-reaction models and integrating diffusion functions into models of synapses and cellular networks. In order to adapt NEURON to evolving computer architectures, the compute engine of the NEURON simulator has been extracted and has been optimized as a library called CoreNEURON. This document describes how NEURON-CoreNEURON can be built and used to run benchmarks on CPU/GPU system.

### Benchmark

In order to have flexibility to scale the model on different number/type of nodes, a synthetic multiple ring network model is provided. This model allows to change number of cells, branching patterns, compartments per branch etc. via command line arguments  ([details here](https://github.com/nrnhines/ringtest/blob/master/README.md#performance-benchmarking)). We have configured job script with necessary command line options.

### Source Code

NEURON repository includes necessary dependencies including [CoreNEURON](https://github.com/BlueBrain/CoreNeuron) and [NMODL](https://github.com/BlueBrain/nmodl). You can download the version to be used for running benchmarks as:

```bash
git clone --recursive https://github.com/neuronsimulator/nrn.git -b benchmark/cineca-v1
```

### Build Dependencies

Following software packages are required to build NEURON-CoreNEURON toolchain:

* Flex (>=2.6)
* Bison (>=3.0)
* CMake (>=3.8)
* readline
* ncurses
* MPI (3 support)
* Python (>=3.6)
* Python packages : jinja2 (>=2.10), pyyaml (>=3.13), pytest (>=4.0.0), sympy (>=1.3), textwrap
* C/C++ Compiler (C++14 Support)
* PGI Compiler (for GPU Support using OpenACC, >= 18.0)
* CUDA  (>= CUDA 8.0)

### Building NEURON-CoreNEURON

We have provided the [install.sh](install.sh) script that can be used to build NEURON-CoreNEURON in a typical cluster environment. Make sure to have above mentioned dependencies are installed / loaded via modules. Also, see *TODO* sections in the install script where you have to make possible changes. If necessary, you can also refer to the relevant travis CI configurations on GitHub : [NEURON](https://github.com/neuronsimulator/nrn/blob/master/.travis.yml), [CoreNEURON](https://github.com/BlueBrain/CoreNeuron/blob/master/.travis.yml) and [NMODL](https://github.com/BlueBrain/nmodl/blob/master/.travis.yml).

### Running Benchmarks

We have provided [job.sh](job.sh) script that is used for benchmarking in weak scaling scenario. Here are the important parameters that one can tune based on time / memory limits :

* **SIM\_TIME** : This is a biological simulation time in milliseconds. You can increase/decrease this to meet the desired runtime of the benchmark.
* **MAX\_NUM\_GPUS\_PER\_NODE** : This is a maximum number of GPUs available on a single node. This parameter is only used to decide the problem size per node and hence can remain constant for a given system.
* **NUM\_GPUS\_PER\_NODE** : This is a number of GPUs allocated per node for a given job. We use `CUDA_VISIBLE_DEVICES` environmental variable to determine GPU count. Change this based on your system configuration.
* **RINGS\_PER\_GPU** : This parameter is also used to calculate problem size per GPU. This parameter should remain constant but you can change it if you want to increase or decrease memory usage per CPU/GPU node.
* **INSTALL\_DIR** : This is a top level installation directory created by installer script [install.sh](install.sh).

See *TODO* sections in the *job.sh* script where you have to make possible changes. When a job is successfully finished, it will print ***Solver Time*** that is used for comparing performance.

### Reference Performance Results

We ran the benchmark on a small cluster using 4 GPUs per node. Each node has following configuration:

* 2 x Intel Xeon 6140 (2 x 18 cores)
* 4 NVIDIA V100 SXM2 (16GB)

We used following simulation parameters:

* **SIM\_TIME** : 50ms
* **MAX\_NUM\_GPUS\_PER\_NODE** : 4
* **RINGS\_PER\_GPU** : 256

The simulation times (second) in a weak scaling scenario are as follows:

#### CPU Results

| #Nodes   | Time (Sec)|
|----------|---------|
|  1       | 67.71   |
|  2       | 67.61   |
|  4       | 68.20   |

For CPU execution we used 36 ranks per node i.e. 1 process per node.

#### GPU Results

| #Nodes  | Time (Sec) (1 GPU per node)  | Time (Sec) (2 GPU per node) | Time (Sec) (4 GPU per node)  |
|---------|------------------------------|-----------------------------|------------------------------|
| 1       | 15.26                        | 7.83                        | 4.62                         |
| 2       | 15.29                        | 7.85                        | 4.65                         |
| 4       | 15.33                        | 7.89                        | 4.68                         |

For GPU execution we used 1 rank per GPU node.

### Question / Help

NEURON-CoreNEURON is developed on GitHub. You can reach to developers via GitHub issues:

* NEURON : [https://github.com/neuronsimulator/nrn](https://github.com/neuronsimulator/nrn)
* CoreNEURON : [https://github.com/BlueBrain/CoreNeuron](https://github.com/BlueBrain/CoreNeuron)