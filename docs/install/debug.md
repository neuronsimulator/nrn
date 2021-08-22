
Diagnosis and Debugging
-----------------------

Disorganized and incomplete but here is a place to put some sometimes
hard-won hints for Diagnosis

Debugging is as close as we get to the scientific method in program
development. From reality not corresponding to expectation, a hypothesis,
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

#### Floating point DIVBYZERO, INVALID, OVERFLOW, exp(700)

#### Different results with different nhost or nthread.
What is the gid and spiketime of the earliest difference?
Use :meth:`ParallelContext.prcellstate` for that gid at various times
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

#### Valgrind

#### Valgrind + gdb
```
valgrind --vgdb=yes --vgdb-error=0 `pyenv which python` test.py''
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

