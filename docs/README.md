## NEURON Documentation

### Online
Latest NEURON documentation is available on:
*  ReadTheDocs @ [https://nrn.readthedocs.io/en/latest/](https://nrn.readthedocs.io/en/latest/).
* GithubPages @ [https://neuronsimulator.github.io/nrn/](https://neuronsimulator.github.io/nrn/).

Contents:
* Documentation on building NEURON
* User documentation (HOC, Python, tutorials, rxd)
* Developer documentation (SCM, technical topics, Doxygen)

### Local build

#### Virtual environment
It is recommended to use a Python virtual environment, for example:

```bash
python3 -m venv venv
source venv/bin/activate
```


In order to build documentation locally, you need to pip install the ``docs_requirements.txt`` :
```
pip3 install -r docs/docs_requirements.txt --upgrade
```

Also, make sure to have `Doxygen` and `pandoc` installed, and the dependencies listed in [conda_environment.yml](conda_environment.yml)
Note that this conda environment file is tailored to the online ReadTheDocs setup (but it lists out all desired requirements, so make sure to check it out).

#### Anaconda environment

After installing Anaconda, create a new environment with the following command:

```bash
conda env create --quiet --name rtd --file docs/conda_environment.yml
conda activate rtd
```

This will install all dependencies needed to build the documentation locally, in a similar way as on ReadTheDocs. However ReadTheDocs has a different setup, so it is of interest to head over and check the build logs for additional information.

#### Confguring the build

With all dependencies installed, configure project with CMake (>= v3.17) as described in [CMake Build Options](./cmake_doc/options.rst#nrn-enable-docs-bool-off).

e.g. in your CMake build folder:

```
cmake .. -DNRN_ENABLE_DOCS=ON
```

#### Building the documentation

Note that executing the jupyter notebooks requires working NEURON installation. So make sure the neuron python
module can be imported. For this, you can build and install neuron from source first or you could also do a
`pip install neuron` (in case you don't want/need to compile NEURON from source).

In order to build the full documentation (Doxygen, Jupyter notebooks, Sphinx):
```
make docs
```
That will build everything in the `nrn/docs/_build` folder from where you can open `index.html` locally.

In case you just want the Sphinx build to be performed(i.e. you are not working on Jupyter notebooks or doxygen):
```
make sphinx
```

#### Faster Local Iterations

When working locally on documentation, depending on what you work on, be aware of the following targets to speed up building process:

* `doxygen` 			- build the API documentation only. Ends up in ``_generated`` folder under ``docs``.
* `notebooks` 			- execute & embed outputs in-place into jupyter notebooks, see [notebooks.sh](notebooks.sh)
* `notebooks-clean`     - clears outputs from notebooks. Remember that executing notebooks will add outputs in-place, and we don't want those committed to the repo.
* `sphinx` 				- build Sphinx documentation

**NOTE**:
* `docs` target calls: `doxygen` `notebooks` `sphinx` and `notebooks-clean`.
* `sphinx` target is the one that will assemble all generated output (doxygen, notebooks).

### ReadTheDocs setup
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
* publish the wheels (these will be pip-installed by RTD)
* go to [NRN RTD Versions page](https://readthedocs.org/projects/nrn/versions/) and activate the new tag.  

**NOTE**: build can be re-launched as many times as needed
 
