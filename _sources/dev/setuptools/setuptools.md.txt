# setuptools

## **setup.py**

Traditionally **distutils** has been used, and **setuptools** ended up having its own internal copy. A lot of effort has been put into coping with the deprecation of **distutils** and accomodating **setuptools**. Furthermore, **setup.py** will also be discontinued and we will probably have to move on to **setuptools.build_meta** or find another way to package wheels and perform CMake builds.

 NEURON has several Python extensions
* HOC module (setuptools Extension with our CMake sauce on top)
* three Rx3D extensions (Cython extensions)
* MUSIC (Cython extension)

## **Operational Modes**

We use [setup.py](../../../setup.py) in two operational modes
:

1) **wheel building**
    It boils down to 

        setup.py build_ext bdist_wheel
    
    We configure the HOC extension along with a CMake configure, build all extensios and collect them for the wheel. This is called via [build_wheels.bash](../../../packaging/python/build_wheels.bash).

2) **CMake build**
    It boils down to
    
        setup.py build_ext build  

    We provide the cmake build folder, in this mode we do not run CMake configure, we build all extensions and make sure they are integrated into the CMake build and install. This is called via CMake in [src/nrnpython/CMakeLists.txt](../../../src/nrnpython/CMakeLists.txt) by passing **--cmake-build-dir** (the folder where we configured NEURON with CMake), along with other CMake options. 


## **Activity Diagram**

![](images/setup-py.png)