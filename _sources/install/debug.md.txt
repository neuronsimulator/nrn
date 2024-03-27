
Diagnosis and Debugging
-----------------------

Disorganized and incomplete but here is a place to put some sometimes
hard-won hints for Diagnosis

Debugging is as close as I get to application of the scientific method.
From reality not corresponding to expectation, a hypothesis,
or wild guess based on what is already known about the lack of correspondence,
is used to generate a computational experiment such that result vs
prediction inspires generation of a more detailed hypothesis with a now
more obvious experiment that focuses attention more on the underlying problem.

#### Segfault and crash.

Begin with [GDB](#GDB) to quickly find where the segfault or crash occurred.
If the underlying cause is resistent to variable inspection, e.g. corrupted
memory by unknown other program statements having nothing to do with
what is going on the location of the crash, [Valgrind](#Valgrind) is an
extremely powerful tool, but at the cost of one or two orders of
magnitude slowdown in running the program.
If valgrind is too slow and you cannot reduce the size or simulation time
while continuing to experience the error, it may be worthwhile to look into
[LLVM address sanitizer](https://github.com/neuronsimulator/nrn/issues/1213).

#### NaN or Inf values
Use [h.nrn_feenableexcept(1)](../python/programming/errors.rst#nrn_feenableexcept)
to generate floating point exception for
DIVBYZERO, INVALID, OVERFLOW, exp(700). [GDB](#GDB) can then be used to show
where the SIGFPE occurred.

#### Different results with different nhost or nthread.
What is the gid and spiketime of the earliest difference?
Use [ParallelContext.prcellstate](../python/modelspec/programmatic/network/parcon.rst#ParallelContext.prcellstate)
for that gid at various times
before spiketime to see why and when the prcellstate files become different.
Time 0 after initialization is often a good place to start.

#### GDB
If you normally run with ```python args``` and get a segfault...
Build NEURON with ```-DCMAKE_BUILD_TYPE=Debug```. This avoids
optimization so that all local variables are available for inspection.
```
gdb `pyenv which python`
run args
bt # backtrace
```
There are many gdb tutorials and reference materials. For mac, lldb is
available. See <https://lldb.llvm.org/use/map.html>

Particularly useful commands are bt (backtrace), p (print), b (break), watch,
c (continue), run, n (next)
E.g ```watch -l ps->osrc_``` to watch a variable outside the local scope where
the PreSyn is locally declared.

#### GDB and MPI
If working on a personal computer (not a cluster) and a small number of ranks,
using mpirun to launch a small number of terminals that run serial gdb
has proven effective.
```
mpirun -np 4 xterm -e gdb `pyenv which python`
```

#### Valgrind
Extremely useful in debugging memory errors and memory leaks.

With recent versions of Valgrind (e.g. 3.17) and Python (e.g. 3.9) the
large number of `Invalid read...` errors which obscure the substantive errors,
can be eliminated with `export PYTHONMALLOC=malloc`

```
export PYTHONMALLOC=malloc
valgrind `pyenv which python` -c 'from neuron import h'
    ==47683== Memcheck, a memory error detector
    ==47683== Copyright (C) 2002-2017, and GNU GPL'd, by Julian Seward et al.
    ==47683== Using Valgrind-3.17.0 and LibVEX; rerun with -h for copyright info
    ==47683== Command: /home/hines/.pyenv/versions/3.9.0/bin/python -c from\ neuron\ import\ h
    ==47683== 
    ==47683== 
    ==47683== HEAP SUMMARY:
    ==47683==     in use at exit: 4,988,809 bytes in 25,971 blocks
    ==47683==   total heap usage: 344,512 allocs, 318,541 frees, 52,095,055 bytes allocated
    ==47683== 
    ==47683== LEAK SUMMARY:
    ==47683==    definitely lost: 1,991 bytes in 20 blocks
    ==47683==    indirectly lost: 520 bytes in 8 blocks
    ==47683==      possibly lost: 2,971,659 bytes in 19,446 blocks
    ==47683==    still reachable: 2,014,639 bytes in 6,497 blocks
    ==47683==                       of which reachable via heuristic:
    ==47683==                         newarray           : 96,320 bytes in 4 blocks
    ==47683==         suppressed: 0 bytes in 0 blocks
    ==47683== Rerun with --leak-check=full to see details of leaked memory
    ==47683== 
    ==47683== For lists of detected and suppressed errors, rerun with: -s
    ==47683== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

With respect to memory leaks, we are most interested in keeping the
`definitely lost: 1,991 bytes in 20 blocks` to as low a value as possible
and especially fix memory leaks that increase when code is executed
multiple times. To this end, the useful valgrind args are
`--leak-check=full --show-leak-kinds=definite`
 

#### Valgrind + gdb
```
valgrind --vgdb=yes --vgdb-error=0 `pyenv which python` test.py
```
Valgrind will stop with a message like:
```
==31925== TO DEBUG THIS PROCESS USING GDB: start GDB like this
==31925==   /path/to/gdb /home/hines/.pyenv/versions/3.7.6/bin/python
==31925== and then give GDB the following command
==31925==   target remote | /usr/local/lib/valgrind/../../bin/vgdb --pid=31925
==31925== --pid is optional if only one valgrind process is running
```
In another shell, start GDB:
```
gdb `pyenv which python`
```
Then copy/paste the above 'target remote' command to gdb:
```
target remote | /usr/local/lib/valgrind/../../bin/vgdb --pid=31925
```
Every press of the 'c' key in the gdb shell will move to the location of
the next valgrind error.


#### Sanitizers
The `AddressSanitizer` (ASan), `LeakSanitizer` (LSan), `ThreadSanitizer` (TSan)
and `UndefinedBehaviorSanitizer` (UBSan) are a collection of tools
that rely on compiler instrumentation to catch dangerous behaviour at runtime.
Compiler support is widespread but not ubiquitous, but both Clang and GCC
provide support.

They are all enabled by passing extra compiler options (typically
`-fsanitize=XXX`), and can be steered at runtime using environment variables
(typically `{...}SAN_OPTIONS=...`).
ASan in particular requires that its runtime library is loaded very early during
execution -- this is typically not a problem if you are directly launching an
executable that has been built with ASan enabled, but more care is required if
the launched executable was not built with ASan.
The typical example of this case is loading NEURON from Python, where the
`python` executable is not built with ASan.

As of [#1842](https://github.com/neuronsimulator/nrn/pull/1842), the NEURON
build system is aware of ASan, LSan and UBSan, and it tries to configure the
sanitizers correctly based on the `NRN_SANITIZERS` CMake variable.
In [#2034](https://github.com/neuronsimulator/nrn/pull/2034) this was extended
to TSan via `-DNRN_SANITIZERS=thread`, and support for GCC was added.
For example, `cmake -DNRN_SANITIZERS=address,leak ...` will enable ASan and
LSan, while `-DNRN_SANITIZERS=undefined` will enable UBSan.
Not all combinations of sanitizers are possible, so far ASan, ASan+LSan, TSan
and UBSan have been tested with Clang.
Support for standalone LSan should be possible without major difficulties, but
is not yet implemented.

Depending on your system, you may also need to set the `LLVM_SYMBOLIZER_PATH`
variable to point to the `llvm-symbolizer` executable matching your Clang.
On systems where multiple LLVM versions are installed, such as Ubuntu, this may
need to be the path to the actual executable named `llvm-symbolizer` and not the
path to a versioned symlink such as `llvm-symbolizer-14`.

NEURON should automatically add the relevant compiler flags and configure the
CTest suite with the extra environment variables that are needed; `ctest` should
work as expected.

If `NRN_SANITIZERS` is set then an additional helper script will be written to
`bin/nrn-enable-sanitizer` under the build and installation directories.
This contains the relevant environment variable settings, and if `--preload` is
passed as the first argument then it also sets `LD_PRELOAD` to point to the
relevant sanitizer runtime library.
For example if you have a NEURON installation with sanitizers enabled, you might
use `nrn-enable-sanitizer special -python path/to/script.py` or
`nrn-enable-sanitizer --preload python path/to/script.py` (`--preload` required
because the `python` binary is (presumably) not linked against the sanitizer
runtime library.

LSan, TSan and UBSan support suppression files, which can be used to prevent
tests failing due to known issues.
NEURON includes a suppression file for TSan under `.sanitizers/thread.supp` and
one for UBSan under `.sanitizers/undefined.supp` in the GitHub repository, no
LSan equivalent exists for the moment.

Note that LSan and MPI implementations typically do not play nicely together, so
if you want to use LSan with NEURON, you may need to disable MPI or add a
suppression file that is tuned to your MPI implementation.

Similarly, TSan does not work very well with MPI and (especially) OpenMP
implementations that were not compiled with TSan instrumentation (which they
are typically not).

The GitHub Actions CI for NEURON at the time of writing includes three jobs
using Ubuntu 22.04: ASan (but not LSan) using Clang 14, UBSan using Clang 14,
and TSan using GCC 12.
In addition, there is a macOS-based ASan build using AppleClang, which has the
advantage that it uses `libc++` instead of `libstdc++`.

NMODL supports the sanitizers in a similar way, but this has to be enabled
explicitly: `-DNRN_SANITIZERS=undefined` will not compile NMODL code with UBSan
enabled, you must additionally pass `-DNMODL_SANITIZERS=undefined` to enable
instrumentation of NMODL code.

Profiling and performance benchmarking
--------------------------------------

NEURON has recently gained built-in support for performance profilers. Many modern profilers provide APIs for instrumenting code. This allows the profiler to enable timers or performance counters and store results into appropriate data structures. For implementation details of the generic profiler interface check `src/utils/profile/profiler_interface.h` NEURON now supports following profilers:

* [Caliper](https://software.llnl.gov/Caliper/)
* [likwid](https://github.com/RRZE-HPC/likwid)
* [tau](https://www.paratools.com/tau)*
* [craypat](https://docs.nersc.gov/tools/performance/craypat/)*

*to use this profiler some additional changes to the CMake files might be needed.

#### Instrumentation

Many performance-critical regions of the NEURON codebase have been instrumented
for profiling.
The existing regions have been given the same names as in CoreNEURON to allow
side-by-side comparision when running simulations with and without CoreNEURON
enabled.
More regions can easily be added into the code in one of two ways:

1. using calls to `phase_begin()`, `phase_end()`

```c++
void some_function() {
    nrn::Instrumentor::phase_begin("some_function");
    // code to be benchmarked
    nrn::Instrumentor::phase_end("some_function");
}
```

2. using scoped automatic variables

```c++
void some_function() {
    // unrelated code
    {
        nrn::Instrumentor::phase p("critical_region");
        // code to be benchmarked
    }
    // more unrelated code
}
```
Note: Don't forget to include the profiler header in the respective source files.

#### Running benchmarks

To enable a profiler, one needs to rebuild NEURON with the appropriate flags set. Here is how one would build NEURON with Caliper enabled:

```bash
mkdir build && cd build
cmake .. -DNRN_ENABLE_PROFILING=ON -DNRN_PROFILER=caliper -DCMAKE_PREFIX_PATH=/path/to/caliper/share/cmake/caliper -DNRN_ENABLE_TESTS=ON
cmake --build . --parallel
```
or if you are building CoreNEURON standalone:
```bash
mkdir build && cd build
cmake .. -DCORENRN_ENABLE_CALIPER_PROFILING=ON -DCORENRN_ENABLE_UNIT_TESTS=ON
cmake --build . --parallel
```
in both cases you might need to add something like `/path/to/caliper/share/cmake/caliper` to the `CMAKE_PREFIX_PATH` variable to help CMake find your installed version of Caliper.

Now, one can easily benchmark the default ringtest by prepending the proper Caliper environment variable, as described [here](https://software.llnl.gov/Caliper/CaliperBasics.html#region-profiling).

```bash
$ CALI_CONFIG=runtime-report,calc.inclusive nrniv ring.hoc
NEURON -- VERSION 8.0a-693-gabe0abaac+ magkanar/instrumentation (abe0abaac+) 2021-10-12
Duke, Yale, and the BlueBrain Project -- Copyright 1984-2021
See http://neuron.yale.edu/neuron/credits

Path                     Min time/rank Max time/rank Avg time/rank Time %    
psolve                        0.145432      0.145432      0.145432 99.648498 
  spike-exchange              0.000002      0.000002      0.000002  0.001370 
  timestep                    0.142800      0.142800      0.142800 97.845079 
    state-update              0.030670      0.030670      0.030670 21.014766 
      state-IClamp            0.001479      0.001479      0.001479  1.013395 
      state-hh                0.002913      0.002913      0.002913  1.995957 
      state-ExpSyn            0.002908      0.002908      0.002908  1.992531 
      state-pas               0.003067      0.003067      0.003067  2.101477 
    update                    0.003941      0.003941      0.003941  2.700332 
    second-order-cur          0.002994      0.002994      0.002994  2.051458 
    matrix-solver             0.006994      0.006994      0.006994  4.792216 
    setup-tree-matrix         0.062940      0.062940      0.062940 43.125835 
      cur-IClamp              0.003172      0.003172      0.003172  2.173421 
      cur-hh                  0.007137      0.007137      0.007137  4.890198 
      cur-ExpSyn              0.007100      0.007100      0.007100  4.864846 
      cur-k_ion               0.003138      0.003138      0.003138  2.150125 
      cur-na_ion              0.003269      0.003269      0.003269  2.239885 
      cur-pas                 0.007921      0.007921      0.007921  5.427387 
    deliver-events            0.013076      0.013076      0.013076  8.959540 
      net-receive-ExpSyn      0.000021      0.000021      0.000021  0.014389 
      spike-exchange          0.000037      0.000037      0.000037  0.025352 
      check-threshold         0.003804      0.003804      0.003804  2.606461 
finitialize                   0.000235      0.000235      0.000235  0.161020 
  spike-exchange              0.000002      0.000002      0.000002  0.001370 
  setup-tree-matrix           0.000022      0.000022      0.000022  0.015074 
    cur-hh                    0.000003      0.000003      0.000003  0.002056 
    cur-ExpSyn                0.000001      0.000001      0.000001  0.000685 
    cur-IClamp                0.000001      0.000001      0.000001  0.000685 
    cur-k_ion                 0.000001      0.000001      0.000001  0.000685 
    cur-na_ion                0.000002      0.000002      0.000002  0.001370 
    cur-pas                   0.000002      0.000002      0.000002  0.001370 
  gap-v-transfer              0.000003      0.000003      0.000003  0.002056
```

#### Running GPU benchmarks

Caliper can also be configured to generate [NVTX](https://nvtx.readthedocs.io/en/latest/) annotations for instrumented code regions, which is useful for profiling GPU execution using NVIDIA's tools.
In a GPU build with Caliper support (`-DCORENRN_ENABLE_GPU=ON -DNRN_ENABLE_PROFILING=ON -DNRN_PROFILER=caliper`), you can enable NVTX annotations at runtime by adding `nvtx` to the `CALI_CONFIG` environment variable.

A complete prefix to profile a CoreNEURON process with NVIDIA Nsight Systems could be:

```bash
 nsys profile --env-var NSYS_NVTX_PROFILER_REGISTER_ONLY=0,CALI_CONFIG=nvtx,OMP_NUM_THREADS=1 --stats=true --cuda-um-gpu-page-faults=true --cuda-um-cpu-page-faults=true --trace=cuda,nvtx,openacc,openmp,osrt --capture-range=nvtx --nvtx-capture=simulation <exe>
```
where `NSYS_NVTX_PROFILER_REGISTER_ONLY=0` is required because Caliper does not use NVTX registered string APIs. The `<exe>` command is likely to be something similar to

```bash
# with python
path/to/x86_64/special -python your_sim.py
# or, with hoc
path/to/x86_64/special your_sim.hoc
# or, if you are executing coreneuron directly
path/to/x86_64/special-core --datpath path/to/input/data --gpu --tstop 1
```

You may like to experiment with setting `OMP_NUM_THREADS` to a value larger than
1, but the profiling tools can struggle if there are too many CPU threads
launching GPU kernels in parallel.

For a more detailed analysis of a certain kernel you can use [NVIDIA Nsight
Compute](https://developer.nvidia.com/nsight-compute). This is a kernel profiler
for applications executed on NVIDIA GPUs and supports the OpenACC and OpenMP
backends of CoreNEURON.
This tool provides more detailed information in a nice graphical environment
about the kernel execution on the GPU including metrics like SM throughput, memory
bandwidth utilization, automatic roofline model generation, etc.
To provide all this information the tool needs to rerun the kernel you're interested in
multiple times, which makes execution ~20-30 times slower.
For this reason we recommend running first Caliper with Nsight Systems to find
the name of the kernel you're interested in and then select on this kernel for
analysis with Nsight Compute.
In case you're interested in multiple kernels you can relaunch Nsight Compute with the other
kernels separately.

To launch Nsight Compute you can use the following command:
```bash
ncu -k <kernel_name> --profile-from-start=off --target-processes all --set <section_set> <exe>
```

where
* `kernel_name`: The name of the kernel you want to profile. You may also provide a regex with `regex:<name>`.
* `section_set`: Provides set of sections of the kernel you want to be analyzed. To get the list of sections you can run `ncu --list-sets`.

The most commonly used is `detailed` which provides most information about the kernel execution and `full` which provides all the details
about the kernel execution and memory utilization in the GPU but takes more time to run.
For more information about Nsight Compute options you can consult the [Nsight Compute Documentation](https://docs.nvidia.com/nsight-compute/2021.3/NsightComputeCli/index.html).

Notes:
- CoreNEURON allocates a large number of small objects separately in CUDA
  managed memory to record the state of the Random123 pseudorandom number
  generator. This makes the Nsight Compute profiler very slow, and makes it
  impractical to use Nsight Systems during the initialization phase. If the
  Boost library is available then CoreNEURON will use a memory pool for these
  small objects to dramatically reduce the number of calls to the CUDA runtime
  API to allocate managed memory. It is, therefore, highly recommended to make
  Boost available when using GPU profiling tools.
