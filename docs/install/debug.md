
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
Use [h.nrn_feenableexcept(1)](../python/programming/errors.html#nrn_fenableexcept)
to generate floating point exception for
DIVBYZERO, INVALID, OVERFLOW, exp(700). [GDB](#GDB) can then be used to show
where the SIGFPE occurred.

#### Different results with different nhost or nthread.
What is the gid and spiketime of the earliest difference?
Use [ParallelContext.prcellstate](../python/modelspec/programmatic/network/parcon.html?highlight=prcellstate#ParallelContext.prcellstate)
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


#### ThreadSanitizer (TSAN)
`ThreadSanitizer` is a tool that detects data races. Be aware that a slowdown is incurred by using ThreadSanitizer of about 5x-15x, with typical memory overhead of about 5x-10x.  

Here is how to enable it:
```
cmake ... -DNRN_ENABLE_TESTS=ON -DCMAKE_C_FLAGS="-O0 -fno-inline -g -fsanitize=thread" -DCMAKE_CXX_FLAGS="-O0 -fno-inline -g -fsanitize=thread" ..
```
You can then target a specific test, i.e. `ctest -VV -R ringtest` and have a look at the generated output, for example:
```
94: WARNING: ThreadSanitizer: data race (pid=2572)
94:   Read of size 8 at 0x7b3c00000bf0 by thread T1:
94:     #0 Cvode::at_time(double, NrnThread*) /home/savulesc/Workspace/nrn/src/nrncvode/cvodeobj.cpp:751 (libnrniv.so+0x38673e)
94:     #1 at_time /home/savulesc/Workspace/nrn/src/nrncvode/cvodestb.cpp:133 (libnrniv.so+0x389e27)
94:     #2 _nrn_current__IClamp /home/savulesc/Workspace/nrn/src/nrnoc/stim.c:266 (libnrniv.so+0x5b8f02)
94:     #3 _nrn_cur__IClamp /home/savulesc/Workspace/nrn/src/nrnoc/stim.c:306 (libnrniv.so+0x5b9236)
94:     #4 Cvode::rhs_memb(CvMembList*, NrnThread*) /home/savulesc/Workspace/nrn/src/nrncvode/cvtrset.cpp:68 (libnrniv.so+0x38a0eb)
94:     #5 Cvode::rhs(NrnThread*) /home/savulesc/Workspace/nrn/src/nrncvode/cvtrset.cpp:35 (libnrniv.so+0x38a2f6)
94:     #6 Cvode::fun_thread_transfer_part2(double*, NrnThread*) /home/savulesc/Workspace/nrn/src/nrncvode/occvode.cpp:671 (libnrniv.so+0x3bbbf1)
94:     #7 Cvode::fun_thread(double, double*, double*, NrnThread*) /home/savulesc/Workspace/nrn/src/nrncvode/occvode.cpp:639 (libnrniv.so+0x3bd049)
94:     #8 f_thread /home/savulesc/Workspace/nrn/src/nrncvode/cvodeobj.cpp:1532 (libnrniv.so+0x384f45)
94:     #9 slave_main /home/savulesc/Workspace/nrn/src/nrnoc/multicore.cpp:337 (libnrniv.so+0x5157ee)
94: 
94:   Previous write of size 8 at 0x7b3c00000bf0 by main thread:
94:     #0 Cvode::at_time(double, NrnThread*) /home/savulesc/Workspace/nrn/src/nrncvode/cvodeobj.cpp:753 (libnrniv.so+0x386759)
94:     #1 at_time /home/savulesc/Workspace/nrn/src/nrncvode/cvodestb.cpp:133 (libnrniv.so+0x389e27)
94:     #2 _nrn_current__IClamp /home/savulesc/Workspace/nrn/src/nrnoc/stim.c:266 (libnrniv.so+0x5b8f02)
94:     #3 _nrn_cur__IClamp /home/savulesc/Workspace/nrn/src/nrnoc/stim.c:306 (libnrniv.so+0x5b9236)
94:     #4 Cvode::rhs_memb(CvMembList*, NrnThread*) /home/savulesc/Workspace/nrn/src/nrncvode/cvtrset.cpp:68 (libnrniv.so+0x38a0eb)
94:     #5 Cvode::rhs(NrnThread*) /home/savulesc/Workspace/nrn/src/nrncvode/cvtrset.cpp:35 (libnrniv.so+0x38a2f6)
94:     #6 Cvode::fun_thread_transfer_part2(double*, NrnThread*) /home/savulesc/Workspace/nrn/src/nrncvode/occvode.cpp:671 (libnrniv.so+0x3bbbf1)
..............................................................
94: SUMMARY: ThreadSanitizer: data race /home/savulesc/Workspace/nrn/src/nrncvode/cvodeobj.cpp:751 in Cvode::at_time(double, NrnThread*)
94: ==================
```


Profiling and performance benchmarking
--------------------------------------

NEURON has recently gained built-in support for performance profilers. Many modern profilers provide APIs for instrumenting code. This allows the profiler to enable timers or performance counters and store results into appropriate data structures. For implementation details of the generic profiler interface check `src/utils/profile/profiler_interface.h` NEURON now supports following profilers:

* [Caliper](https://software.llnl.gov/Caliper/)
* [likwid](https://github.com/RRZE-HPC/likwid)
* [tau](https://www.paratools.com/tau)*
* [craypat](https://docs.nersc.gov/tools/performance/craypat/)*

*to use this profiler some additional changes to the CMake files might be needed.

#### Instrumentation

NEURONs code has been already instrumented with instrumentor regions in many performance-critical functions of the code. The existing regions have been given the same names as in CoreNEURON to allow side-by-side comparision when running simulations with and without CoreNEURON enabled. More regions can easily be added into the code in one of two ways:

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
make
```

Now, one can easily benchmark the default ringtest by prepending the proper Caliper environment variable, as described [here](https://software.llnl.gov/Caliper/CaliperBasics.html#region-profiling).

```
$ CALI_CONFIG=runtime-report,calc.inclusive NRNHOME=/Users/awile/projects/cellular/nrn/build2 NEURONHOME=/Users/awile/projects/cellular/nrn/build2/share/nrn /Users/awile/projects/cellular/nrn/build2/bin/nrniv ring.hoc
NEURON -- VERSION 8.0a-658-g07fc295af+ enh/1421 (07fc295af+) 2021-09-07
Duke, Yale, and the BlueBrain Project -- Copyright 1984-2021
See http://neuron.yale.edu/neuron/credits

Path                  Time (E) Time (I) Time % (E) Time % (I)
psolve                0.022743 1.047956   2.119599  97.667258
  timestep            0.189271 1.025213  17.639652  95.547659
    state-update      0.116918 0.208878  10.896508  19.466983
      state-IClamp    0.012888 0.012888   1.201134   1.201134
      state-hh        0.026252 0.026252   2.446630   2.446630
      state-ExpSyn    0.026171 0.026171   2.439081   2.439081
      state-pas       0.026649 0.026649   2.483630   2.483630
    update            0.027531 0.027531   2.565830   2.565830
    second_order_cur  0.026720 0.026720   2.490247   2.490247
    matrix-solver     0.031381 0.031381   2.924642   2.924642
    setup_tree_matrix 0.240728 0.483874  22.435335  45.096022
      cur-IClamp      0.026415 0.026415   2.461821   2.461821
      cur-hh          0.053054 0.053054   4.944519   4.944519
      cur-ExpSyn      0.053927 0.053927   5.025881   5.025881
      cur-k_ion       0.026331 0.026331   2.453993   2.453993
      cur-na_ion      0.026849 0.026849   2.502269   2.502269
      cur-pas         0.056570 0.056570   5.272203   5.272203
    deliver_events    0.057558 0.057558   5.364282   5.364282
finitialize           0.000655 0.000881   0.061045   0.082107
  setup_tree_matrix   0.000120 0.000226   0.011184   0.021063
    cur-hh            0.000022 0.000022   0.002050   0.002050
    cur-ExpSyn        0.000023 0.000023   0.002144   0.002144
    cur-IClamp        0.000011 0.000011   0.001025   0.001025
    cur-k_ion         0.000010 0.000010   0.000932   0.000932
    cur-na_ion        0.000013 0.000013   0.001212   0.001212
    cur-pas           0.000027 0.000027   0.002516   0.002516
```



