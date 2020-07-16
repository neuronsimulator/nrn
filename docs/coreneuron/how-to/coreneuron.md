# NEURON - CoreNEURON Integration

**CoreNEURON** is a compute engine for the **NEURON** simulator optimised for both memory usage and computational speed. Its goal is to simulate large cell networks with minimal memory footprint and optimal performance.

If you are a new user and would like to use **CoreNEURON**, this tutorial will be a good starting point to understand complete workflow of using **CoreNEURON** with **NEURON**.


# How to install CoreNEURON

**CoreNEURON** is a submodule of the NEURON repository and can be installed with NEURON by enabling the flag **NRN_ENABLE_CORENEURON** in the CMake command you are using to install NEURON.
For example:

    cmake .. \
         -DNRN_ENABLE_INTERVIEWS=OFF \
         -DNRN_ENABLE_MPI=OFF \
         -DNRN_ENABLE_RX3D=OFF \
         -DNRN_ENABLE_CORENEURON=ON

## Notes

### Performance
**CoreNEURON** is by itself an optimized compute engine of **NEURON**, however to unlock the full performance from your system we suggested using either an **Intel Compiler** or the **NMODL ISPC Backend** which we're going to discuss below. Those compilers are able to vectorize better the code than **GCC** achieving the best performance gains possible.

### NMODL
The **NMODL** Framework is a code generation engine for the **N**EURON **MOD**eling **L**anguage. 
**NMODL** is a part of **CoreNEURON** and can be used to generate optimized code for modern compute architectures including CPUs, GPUs.
**NMODL** can also generate code using a backend for the **ISPC** compiler, which can vectorize greatly the generated code running on CPU and accelerate further the simulation.

> Disclaimer: To use the ISPC compiler backend you need to have ISPC compiler preinstalled in your system

#### How to use NMODL
You can enable the **NMODL** code generation in **CoreNEURON** using the following *CMake* command:

       cmake .. \
         -DNRN_ENABLE_INTERVIEWS=OFF \
         -DNRN_ENABLE_MPI=OFF \
         -DNRN_ENABLE_RX3D=OFF \
         -DNRN_ENABLE_CORENEURON=ON \
         -DCORENRN_ENABLE_NMODL=ON
To use the **ISPC** backed you can add the following flags:

    -DCORENRN_ENABLE_ISPC=ON
    -DCMAKE_ISPC_COMPILER=<path-to-ISPC-compiler-installation> # Only if the ispc executable is not in the PATH environment variable
> Disclaimer: NMODL is currently under development and some mod files could be unsupported. Please feel free to try using NMODL with your mod files and report any issues on the [NMODL repository](https://github.com/BlueBrain/nmodl)
# How to use CoreNEURON
## Python API
To use **CoreNEURON** directly from **NEURON** using the in-memory mode you can add the following in your python script before calling the *psolve* function to run the simulation:

    from neuron import coreneuron
    coreneuron.enable = True

> To run a simulation with coreneuron h.cvode.cache_efficient(1) must be set

You can find an example of the in-memory mode usage of **CoreNEURON** [here](https://github.com/neuronsimulator/nrn/blob/master/test/coreneuron/test_direct.py)
## HOC API
To use the **CoreNEURON** in-memory mode using a HOC script you need to add the following before calling the *psolve* function to run the simulation:

    po = new PythonObject()
    po.coreneuron.enable = 1
    po.coreneuron.nrncore_arg(tstop)
You can find an example of the in-memory mode usage of **CoreNEURON** [here](https://github.com/neuronsimulator/nrn/blob/master/test/coreneuron/test_direct.hoc)
## How to handle extra mod files
In case you want to compile and use extra mod files with **NEURON** or **CoreNEURON**, before using your script you need to run on the same folder **nrnivmodl** as usual and then **nrnivmodl-core**, which is the equivalent of **nrnivmodl** for **CoreNEURON** and takes care of compiling and linking the extra mechanisms' mod files into **CoreNEURON**.
For example:

    nrnivmodl mod
    nrnivmodl-core mod
    ./x86_64/special -python init.py

# CoreNEURON on GPUs
**CoreNEURON** can be also configured to run on GPUs. To do so, you need to specify an extra flag in your *CMake* command:

    -DCORENRN_ENABLE_GPU=ON
Due to technical issues it's not yet possible to run **NEURON** with **CoreNEURON** with the in-memory mode.
To run your simulation on GPUs you need to:

 1. Run **nrnivmodl** and **nrnivmodl-core** as before
 2. Repalce *pc.psolve()* with **pc.nrnbbcore_write('coredat')** and run your script as before
This call is going to generate an intermediate model inside the *coredat* folder which is then going to be used as input to **CoreNEURON**.
3. Run **CoreNEURON** using the **special-core** executable which was created in the **x86_64** folder and specify the **--datpath coredat**. You can find the rest of the options of **special-core**  [here](https://github.com/BlueBrain/CoreNeuron)

An example bash script for running **CoreNEURON** on GPUs is the following:

    nrnivmodl mod
    nrnivmodl-core mod
    ./x86_64/special -python init.py
    ./x86_64/special-core --gpu --datpath coredat -e 10


