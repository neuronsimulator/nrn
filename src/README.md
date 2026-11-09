# NEURON source directory

| Directory | Contents |
| --- | --- |
| [coreneuron/](coreneuron/) | CoreNEURON simulation engine, optimized for large networks, with CPU and GPU support. |
| [gnu/](gnu/) | Random number generators and probability distributions, including Random123 and MCellRan4. |
| [ivoc/](ivoc/) | HOC classes such as Vector, Matrix, and Random, plus InterViews GUI widgets, graphs, and application startup. |
| [ivos/](ivos/) | Minimal InterViews support for builds without the GUI: resource management, observers, and compatibility headers. |
| [mac/](mac/) | macOS application launchers, icons, packaging, code signing, and notarization scripts. |
| [modlunit/](modlunit/) | The `modlunit` tool for checking physical units in NMODL mechanism files. |
| [mswin/](mswin/) | Windows compatibility code, installer scripts, icons, and packaging resources. |
| [neuron/](neuron/) | C++ model data structures, node and mechanism storage, data handles, and cache views. |
| [neuronmusic/](neuronmusic/) | MUSIC integration for exchanging events between NEURON and other simulators, including Python bindings. |
| [nmodl/](nmodl/) | Modern NMODL compiler: parsing, syntax trees, analysis and optimization passes, C++ code generation, and Python bindings. |
| [nocmodl/](nocmodl/) | Traditional `nocmodl` translator from NMODL mechanism files to C++. |
| [node_order_optim/](node_order_optim/) | Tree-node ordering and permutation helpers for improving simulation performance. |
| [nrncvode/](nrncvode/) | Variable time step solver integration, network event delivery, event queues, and recording support. |
| [nrniv/](nrniv/) | Simulator integration: morphology GUI, network parallelism, state saving, CoreNEURON transfer, and the C API; also builds the main executable and library. It also contains impedance analysis (Impedance), kinetic scheme channels (KSChan), LinearMechanism, multisplit, differential-algebraic equation support, and CVODE's NVector implementations. |
| [nrnmpi/](nrnmpi/) | MPI communication wrappers, spike exchange, message packing, and dynamic MPI loading. |
| [nrnoc/](nrnoc/) | Core compartment simulation: sections, cable equations, membrane mechanisms, ions, time stepping, and built-in MOD mechanisms. |
| [nrnpython/](nrnpython/) | Python interpreter integration, HOC/Python and section/segment bindings, and reaction-diffusion (RxD) computational code. This is the C++ part; the Python code lives in `share/lib/python/neuron/` |
| [oc/](oc/) | HOC language parser, interpreter, object system, symbols, built-in functions, and I/O. |
| [parallel/](parallel/) | ParallelContext interface and bulletin-board task scheduling, with local and MPI implementations. |
| [pocketfft/](pocketfft/) | Bundled header-only FFT library used by Vector Fourier transforms. |
| [scopmath/](scopmath/) | SCoP numerical routines used by mechanism code, including ODE solvers, nonlinear and sparse solvers, and mathematical functions. |
| [sparse/](sparse/) | Sparse linear equation solver used by HOC's equation-solving facilities. |
| [sparse13/](sparse13/) | Sparse matrix library for assembly, factorization, and solving, used by simulator matrix and solver code. |
| [sundials/](sundials/) | Bundled CVODES and IDA numerical solvers and shared support routines. |
| [utils/](utils/) | Shared C++ helpers for formatting, logging, signals, iteration, and profiling. |
