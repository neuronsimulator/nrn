## NEURON Development

### Versioning and Release
Please refer to [NEURON Versioning and Release Guidelines](./scm/guidelines/SCMGuidelines.md).

### Source and Release Management Guide
Please refer to [NEURON Source and Release Management Guide](./scm/guide/SCMGuide.md).

### Developer documentation 

#### Online
NEURON consolidated documentation is available at [https://neuronsimulator.github.io/nrn/](https://neuronsimulator.github.io/nrn/):
* user documentation (HOC, Python, tutorials, rxd)
* developer documentation (SCM, technical topics, Doxygen)

#### Local build

It is recommended using a `virtualenv`, for example:

```
pip3 install virtualenv
python3 -m virtualenv venv
source venv/bin/activate
```

In order to build documentation locally, you need to pip install the [docs_requirements](docs_requirements.txt) :
```
pip3 install --user -r docs/docs_requirements.txt --upgrade
```

Also, make sure to have Doxygen installed on your system. With all dependencies installed, configure project with
cmake as described in [README](../README.md). e.g. In your CMake build folder:

```
cmake .. -DNRN_ENABLE_INTERVIEWS=OFF -DNRN_ENABLE_MPI=OFF -DNRN_ENABLE_RX3D=OFF -DCMAKE_INSTALL_PREFIX=`pwd`/install
```

In order to execute the jupyter notebooks, make sure neuron python module can be imported.

Then build docs using command:
```
make docs
```  
That will build everything in the `build/docs` folder and you can then open `index.html` locally. 

When working locally on documentation, be aware of the following targets to speed up building process:

* `doxygen` 			- build the API documentation only
* `notebooks` 			- execute & convert jupyter notebooks to html, see [notebooks.sh](notebooks.sh)
* `notebooks-noexec`	- simply convert jupyter notebooks to html, see [notebooks.sh](notebooks.sh)
* `sphinx` 				- build Sphinx documentation

NOTE: `docs` target calls: `doxygen` `notebooks` `sphinx`. 
