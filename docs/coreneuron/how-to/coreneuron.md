# NEURON - CoreNEURON Integration

**CoreNEURON** is a compute engine for the **NEURON** simulator optimised for both memory usage and computational speed using modern CPU/GPU architetcures. Its goal is to simulate large cell networks with minimal memory footprint and optimal performance.

If you are a new user and would like to use **CoreNEURON**, this tutorial will be a good starting point to understand the complete workflow of using **CoreNEURON** with **NEURON**. We would be grateful for any feedback about your use cases or issues you encounter using the CoreNEURON. Please [report any issue here](https://github.com/neuronsimulator/nrn/issues) and we will be happy to help.


# How to install CoreNEURON

**CoreNEURON** is a submodule of the **NEURON** repository and can be installed with **NEURON** by enabling the flag **NRN_ENABLE_CORENEURON** in the CMake command used to install **NEURON**.
For example:

    cmake .. \
         -DNRN_ENABLE_INTERVIEWS=OFF \
         -DNRN_ENABLE_MPI=OFF \
         -DNRN_ENABLE_RX3D=OFF \
         -DNRN_ENABLE_CORENEURON=ON

## Notes

### Performance
**CoreNEURON** is by itself an optimized compute engine of **NEURON**, however to unlock better CPU performance it's recommended to use compilers like **Intel / PGI / Cray  Compiler** or the **ISPC Backend** using the new [NMODL Compiler Framework](https://github.com/BlueBrain/nmodl) (described described below). These compilers are able to vectorize the code better than **GCC** or **Clang**, achieving the best possible performance gains. Note that Intel compiler can be installed by downloading [oneAPI HPC Toolkit](https://software.intel.com/content/www/us/en/develop/tools/oneapi/hpc-toolkit.html).

### GPU execution
**CoreNEURON** supports also GPU execution based on an **OpenACC** backend. To be able to use it it's needed to install **NEURON** and **CoreNEURON** with a compiler that supports **OpenACC**. Currently the best supported compiler for the **OpenACC** backend is **PGI** (available via [NVIDIA-HPC-SDK](https://developer.nvidia.com/hpc-sdk)) and this is the recommended one for compilation.
To enable the GPU backend specify the following *CMake* variables:

    -DNRN_ENABLE_CORENEURON=ON
    -DCORENRN_ENABLE_GPU=ON

Make sure to set the compilers via CMake flags `-DCMAKE_C_COMPILER=<C_Compiler>` and `-DCMAKE_CXX_COMPILER=<CXX_Compiler>`.

### NMODL

The **NMODL** Framework is a code generation engine for the **N**EURON **MOD**eling **L**anguage. 
**NMODL** is a part of **CoreNEURON** and can be used to generate optimized code for modern compute architectures including CPUs and GPUs.
**NMODL** can also generate code using a backend for the **ISPC** compiler, which can vectorize greatly the generated code running on CPU and accelerate further the simulation.

**NOTE**: To use the ISPC compiler backend ISPC compiler must be preinstalled in your system. More information on how to install ISPC can be found [here](https://ispc.github.io/downloads.html).

#### How to use NMODL
To enable the **NMODL** code generation in **CoreNEURON** specify the following *CMake* variables:

    -DNRN_ENABLE_CORENEURON=ON
    -DCORENRN_ENABLE_NMODL=ON

To use the **ISPC** backend add the following flags:

    -DCORENRN_ENABLE_ISPC=ON
    -DCMAKE_ISPC_COMPILER=<path-to-ISPC-compiler-installation> # Only if the ispc executable is not in the PATH environment variable

**NOTE** : NMODL is currently under active development and some mod files could be unsupported. Please feel free to try using NMODL with your mod files and report any issues on the [NMODL repository](https://github.com/BlueBrain/nmodl)

# How to use CoreNEURON

## Python API
To use **CoreNEURON** directly from **NEURON** using the in-memory mode add the following in your python script before calling the *psolve* function to run the simulation:

    from neuron import coreneuron
    coreneuron.enable = True
    coreneuron.gpu = True # Only for GPU execution

**NOTE**: In order to run a simulation with CoreNEURON, `h.cvode.cache_efficient(1)` must also be set.

You can find an example of the in-memory mode usage of **CoreNEURON** [here](https://github.com/neuronsimulator/nrn/blob/master/test/coreneuron/test_direct.py).

## HOC API
To use the **CoreNEURON** in-memory mode using a HOC script you need to add the following before calling the *psolve* function to run the simulation:

    if (!nrnpython("from neuron import coreneuron")) {
        printf("Python not available\n")
        return
    }

    po = new PythonObject()
    po.coreneuron.enable = 1

**NOTE**: In order to run a simulation with CoreNEURON, h.cvode.cache_efficient(1) must also be set

You can find an example of the in-memory mode usage of **CoreNEURON** [here](https://github.com/neuronsimulator/nrn/blob/master/test/coreneuron/test_direct.hoc)

See more informaiton in CoreNEURON [README documentation](https://github.com/BlueBrain/CoreNeuron#dependencies).

## Compiling MOD files

In order to use mod files with **NEURON** or **CoreNEURON** you need to execute **nrnivmodl** and pass as argument the folder of the mod files you want to compile. To run the simulation using **CoreNEURON**, you have to use *-coreneuron* flag and then you can execute your **special** binary to run the model.
For example:

    nrnivmodl -coreneuron mod_folder
    ./x86_64/special -python init.py

