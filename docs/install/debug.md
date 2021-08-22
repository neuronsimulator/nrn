
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

