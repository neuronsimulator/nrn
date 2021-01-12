# NEURON - CoreNEURON Integration

**CoreNEURON** is a compute engine for the **NEURON** simulator optimised for both memory usage and computational speed. Its goal is to simulate large cell networks with minimal memory footprint and optimal performance.

If you are a new user and would like to use **CoreNEURON**, this tutorial will be a good starting point to understand the complete workflow of using **CoreNEURON** with **NEURON**.


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
**CoreNEURON** is by itself an optimized compute engine of **NEURON**, however to unlock better performance from the used CPUs it's suggested using either an **Intel Compiler** or the **NMODL ISPC Backend** which is described below. Those compilers are able to vectorize better the code than **GCC** achieving the best performance gains possible.

### GPU execution
**CoreNEURON** supports also GPU execution based on an **OpenACC** backend. To be able to use it it's needed to install **NEURON** and **CoreNEURON** with a compiler that supports **OpenACC**. Currently the best supported compiler for the **OpenACC** backend is **PGI** and this is the recommended one for compilation.
To enable the GPU backend specify the following *CMake* variables:

    -DNRN_ENABLE_CORENEURON=ON
    -DCORENRN_ENABLE_GPU=ON

### NMODL
The **NMODL** Framework is a code generation engine for the **N**EURON **MOD**eling **L**anguage. 
**NMODL** is a part of **CoreNEURON** and can be used to generate optimized code for modern compute architectures including CPUs and GPUs.
**NMODL** can also generate code using a backend for the **ISPC** compiler, which can vectorize greatly the generated code running on CPU and accelerate further the simulation.

> Disclaimer: To use the ISPC compiler backend ISPC compiler must be preinstalled in your system. More information on how to install ISPC can be found [here](https://ispc.github.io/downloads.html). 

#### How to use NMODL
To enable the **NMODL** code generation in **CoreNEURON** specify the following *CMake* variables:

    -DNRN_ENABLE_CORENEURON=ON
    -DCORENRN_ENABLE_NMODL=ON

To use the **ISPC** backend add the following flags:

    -DCORENRN_ENABLE_ISPC=ON
    -DCMAKE_ISPC_COMPILER=<path-to-ISPC-compiler-installation> # Only if the ispc executable is not in the PATH environment variable

> Disclaimer: NMODL is currently under development and some mod files could be unsupported. Please feel free to try using NMODL with your mod files and report any issues on the [NMODL repository](https://github.com/BlueBrain/nmodl)

# How to use CoreNEURON
## Python API
To use **CoreNEURON** directly from **NEURON** using the in-memory mode add the following in your python script before calling the *psolve* function to run the simulation:

    from neuron import coreneuron
    coreneuron.enable = True
    coreneuron.gpu = True # Only for GPU execution

> To run a simulation with CoreNEURON h.cvode.cache_efficient(1) must also be set

You can find an example of the in-memory mode usage of **CoreNEURON** [here](https://github.com/neuronsimulator/nrn/blob/master/test/coreneuron/test_direct.py)

## HOC API
To use the **CoreNEURON** in-memory mode using a HOC script you need to add the following before calling the *psolve* function to run the simulation:

    po = new PythonObject()
    po.coreneuron.enable = 1
    po.coreneuron.nrncore_arg(tstop)

> To run a simulation with CoreNEURON h.cvode.cache_efficient(1) must also be set

You can find an example of the in-memory mode usage of **CoreNEURON** [here](https://github.com/neuronsimulator/nrn/blob/master/test/coreneuron/test_direct.hoc)

## How to handle extra mod files
In case you want to compile and use extra mod files with **NEURON** or **CoreNEURON** you need to execute **nrnivmodl** and pass as argument the folder of the mod files you want to compile. To create the libraries and executables needed to run the **CoreNEURON** simulation it's also needed to add the *-coreneuron* flag and then you can execute your **special** and your script that uses **CoreNEURON** as before.
For example:

    nrnivmodl -coreneuron mod
    ./x86_64/special -python init.py
