## NEURON Documentation

### Online

Latest NEURON documentation is available on:

* ReadTheDocs @ [https://nrn.readthedocs.io/en/latest/](https://nrn.readthedocs.io/en/latest/).

Contents:

* Documentation on building NEURON
* User documentation (HOC, Python, tutorials, rxd)
* Developer documentation (SCM, technical topics, Doxygen)

### On pull requests

The documentation is built automatically as part of continuous integration (CI) when a pull request is opened, under the name `docs/readthedocs.org:nrn`.
Upon success, it can viewed by clicking on the "Details" link to the right of the name.
If the build fails, the logs will be available at the same link.

### Local build

To build the NEURON documentation, one needs to install the same prerequisites as for building NEURON itself; see the [install instructions](./install/install_instructions.md) for a complete list of build dependencies.

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

Also, make sure to have `Doxygen` and `pandoc` installed.
On MacOS, you can install these packages using `brew`:

```bash
brew install doxygen pandoc
```

#### Configuring the build

With all dependencies installed, configure project with CMake (>= v3.17) as described in [CMake Build Options](./cmake_doc/options.rst#nrn-enable-docs-bool-off).

e.g. in your CMake build folder:

```
cmake .. -DNRN_ENABLE_DOCS=ON
```

#### Building the documentation

In order to build the full documentation (Doxygen, Jupyter notebooks, Sphinx):
```
make docs
```
That will build everything in the `nrn/docs/_build` folder from where you can open `index.html` locally.

In case you just want the Sphinx build to be performed (i.e. you are not working on Jupyter notebooks or doxygen):
```
make sphinx
```

#### Faster Local Iterations

When working locally on documentation, depending on what you work on, be aware of the following targets to speed up building process:

* `doxygen` 			- build the API documentation only. Ends up in ``_generated`` folder under ``docs``.
* `notebooks` 			- execute & embed outputs in-place into jupyter notebooks, see [PythonNotebookHelper.cmake](../cmake/PythonNotebookHelper.cmake)
* `notebooks-clean`     - clears outputs from notebooks. Remember that executing notebooks will add outputs in-place, and we don't want those committed to the repo.
* `sphinx` 				- build Sphinx documentation

**NOTE**:
* `docs` target calls: `doxygen` `notebooks` `sphinx`.
* `sphinx` target is the one that will assemble all generated output (doxygen, notebooks).

### ReadTheDocs setup

#### Config file

Configuration file is in the top directory: [../.readthedocs.yml](../.readthedocs.yml).

#### Documentation not handled by RTD

For RTD we need to call `doxygen` and `notebooks` make targets, since only `sphinx` executable is called therein.

#### Leveraging `conf.py`

The only way to gather extra documentation in RTD is to make use of `conf.py`.
Have a look at `if os.environ.get("READTHEDOCS"):` block in [conf.py](conf.py) to see how we leverage the aforementioned targets and outputs.

#### Notebooks execution with `neuron` wheels

For `notebooks` we are using `neuron` Python wheel as follows:
* `neuron-nightly` for `master` RTD builds.
* `neuron==X.Y.Z` for `release` RTD builds. Note that wheels need to be published first (see next section).

To achieve this, we parse the RTD environment variable `READTHEDOCS_VERSION` and `pip install` it (see [conf.py](conf.py)).

#### New release on RTD

To create a new public release on RTD, follow these steps:

* create a new release (see [GHA Neuron Release - Manual Workflow](https://github.com/neuronsimulator/nrn/actions?query=workflow%3A%22NEURON+Release%22))
* publish the wheels (these will be pip-installed by RTD)
* go to [NRN RTD Versions page](https://readthedocs.org/projects/nrn/versions/) and activate the new tag. Make sure that the version is not marked as hidden.

**NOTE**: builds can be re-launched as many times as needed
