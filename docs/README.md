## NEURON Development

### Versioning and Release
Please refer to [NEURON Versioning and Release Guidelines](./scm/guidelines/SCMGuidelines.md).

### Source and Release Management Guide
Please refer to [NEURON Source and Release Management Guide](./scm/guide/SCMGuide.md).

### Developer documentation 

#### Online
Latest NEURON documentation is available on:
* GithubPages @ [https://neuronsimulator.github.io/nrn/](https://neuronsimulator.github.io/nrn/).
* ReadTheDocs @ [https://nrn.readthedocs.io/en/latest/](https://nrn.readthedocs.io/en/latest/).

Contents:
* documentation on building NEURON
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

Also, make sure to have installed dependencies listed in [conda_environment.yml](conda_environment.yml).
With all dependencies installed, configure project with CMake as described in [README](../README.md). 

e.g. in your CMake build folder:

```
cmake .. -DNRN_ENABLE_INTERVIEWS=OFF -DNRN_ENABLE_MPI=OFF -DNRN_ENABLE_RX3D=OFF -DCMAKE_INSTALL_PREFIX=`pwd`/install
```

In order to execute the jupyter notebooks, make sure neuron python module can be imported.

Then build docs using command:
```
make docs
```  
That will build everything in the `nrn/docs/_build` folder and you can then open `index.html` locally. 

When working locally on documentation, be aware of the following targets to speed up building process:

* `doxygen` 			- build the API documentation only. Ends up in [_generated](_generated)
* `notebooks` 			- execute & convert jupyter notebooks to html, see [notebooks.sh](notebooks.sh)
* `notebooks-noexec`	- simply convert jupyter notebooks to html, see [notebooks.sh](notebooks.sh)
* `sphinx` 				- build Sphinx documentation

NOTE: `docs` target calls: `doxygen` `notebooks` `sphinx`. 

### ReadTheDocs.io setup
#### Config file
Configuration file is in the top directory: [../.readthedocs.yml](../.readthedocs.yml).

#### Documentation not handled by RTD
For RTD we need to call `doxygen` and `notebooks` make targets, since only `sphinx` executable is called therein.
To that end we are using conda to be able to install packages like CMake in RTD.
All conda dependencies are listed under [conda_environment.yml](conda_environment.yml).

#### Leveraging `conf.py`
The only way to gather extra documentation in RTD is to make use of `conf.py`.
Have a look at `if os.environ.get("READTHEDOCS"):` block in [conf.py](conf.py) to see how we leverage the aforementioned targets and outputs.

#### Notebooks execution with `neuron` wheels
For `notebooks` we are using `neuron` Python wheel as follows:
* `neuron-nightly` for `master` RTD builds.
* `neuron==X.Y.Z` for `release` RTD builds. Note that wheels need to be published first (see next section).

To achieve this, we parse the RTD environment variable `READTHEDOCS_VERSION` and `pip install` it (see [conf.py](conf.py)) .

#### New release on RTD
Follow the following steps:
* create a new release (see [GHA Neuron Release - Manual Workflow](https://github.com/neuronsimulator/nrn/actions?query=workflow%3A%22NEURON+Release%22))
* publish wheels
* go to [NRN RTD Versions page](https://readthedocs.org/projects/nrnalex/versions/) and activate the new tag.  

Note: build can be re-launched as many times as needed
 
