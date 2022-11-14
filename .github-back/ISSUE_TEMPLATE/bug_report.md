---
name: NEURON Bug Report
about: Detailed report to describe NEURON and CMake build related bugs
title: ''
labels: 'bug'
assignees: ''

---

## Context

### Overview of the issue

[Provide more details regarding the issue]

### Expected result/behavior

[Describe what is the expected result and/or behavior]

### NEURON setup
 
 - Version: [e.g. master branch / 7.8.2]
 - Installation method [e.g. cmake build / pip / Windows Installer / macOS installer]
 - OS + Version: [e.g. Ubuntu 20.04]
 - Compiler + Version: [e.g. gcc-9]
 
## Minimal working example - MWE

MWE that can be used for reproducing the issue and testing. A couple of examples:

* python script:
```python
from neuron import h

def test_area():
    soma = h.Section(name="soma")
    soma.L = soma.diam = 10
    assert 314.159 < sum(seg.area() for seg in soma) < 314.16

test_area()
```

* CMake build commands:
```cmake
git clone git@github.com:neuronsimulator/nrn.git
mkdir build && cd build
cmake -DNRN_ENABLE_CORENEURON=ON -DCMAKE_C_COMPILER=icc -DCMAKE_CXX_COMPILER=icpc ..
cmake --build . --
```
## Logs

[If this a build issue `CMakeError.log`, `CMakeOutput.log` or the output of `make VERBOSE=1` would be very helpful.
 Please attach the full file and not its content!
 
 Otherwise provide the error printed to the terminal or a screenshot of the issue]
