# Building Python Wheels

See also [this document](../dev/python/wheels).

## Linux wheels

In order to have NEURON binaries run on most Linux distros, we rely on the [manylinux project](https://github.com/pypa/manylinux).
Current NEURON Linux image is based on `manylinux_2_28`.

### Setting up Docker

[Docker](https://en.wikipedia.org/wiki/Docker_(software)) is required for building Linux wheels.
You can find instructions on how to setup Docker on Linux [here](https://docs.docker.com/engine/install/).


### NEURON Docker Image Workflow

Linux wheel builds run inside
[neuronsimulator/neuron_wheel](https://hub.docker.com/r/neuronsimulator/neuron_wheel).
Azure pipelines pull these tags from Docker Hub:

* `manylinux_2_28_x86_64`
* `manylinux_2_28_aarch64`

`pyproject.toml` sets those names (`manylinux-x86_64-image` and `manylinux-aarch64-image`).

Publish again after a change to [packaging/python/Dockerfile](../../packaging/python/Dockerfile) has been reviewed and merged to `master`. The steps below build that file and push the two tags.

All wheels built on Azure are:

* Published to `Pypi.org` as
  * `neuron-nightly` -> when the pipeline is launched in CRON mode
  * `neuron-x.y.z` -> when the pipeline is manually triggered for release `x.y.z`
* Stored as `Azure artifacts` in the Azure pipeline for every run.

### One-time: store the Docker Hub token

1. Sign in as a Docker Hub user who can push `neuronsimulator/neuron_wheel`.
2. Create a personal access token on that account.
3. In the GitHub settings for `neuronsimulator/nrn`, under Actions, store:
   * variable `DOCKERHUB_USERNAME`: the Docker Hub user name
   * secret `DOCKERHUB_TOKEN`: the personal access token

When `docker login` asks for a password, paste the personal access token.

### Publish from GitHub Actions

1. On GitHub, open **Actions** → **Build custom Docker image for manylinux wheels** → **Run workflow**. Choose the `master` branch.
2. Fill in the form:
   * **The base Docker image to use:** `manylinux_2_28`
   * **Whether to upload (push) the image to the container registry:** off
   * **The name of the container registry:** `docker.io`. The form opens on `ghcr.io`. Select `docker.io`. The wheel build reads the `docker.io` tags.
   * **The tag prefix for the final Docker image:** leave empty
3. Run the workflow. It builds `x86_64` and `aarch64` and does not push.
4. Run it again with upload turned on.

The images are then:

* `docker.io/neuronsimulator/neuron_wheel:manylinux_2_28_x86_64`
* `docker.io/neuronsimulator/neuron_wheel:manylinux_2_28_aarch64`

A prefix is joined to the front of that tag with no extra character. An empty prefix produces the names above.

### Build and push on your own machine

For `x86_64`:

```
cd nrn/packaging/python
docker build -t neuronsimulator/neuron_wheel:manylinux_2_28_x86_64 .
docker login --username=<dockerhub-username>
docker push neuronsimulator/neuron_wheel:manylinux_2_28_x86_64
```

For `aarch64`:

```
cd nrn/packaging/python
docker build -t neuronsimulator/neuron_wheel:manylinux_2_28_aarch64 --build-arg MANYLINUX_IMAGE=manylinux_2_28_aarch64 .
docker login --username=<dockerhub-username>
docker push neuronsimulator/neuron_wheel:manylinux_2_28_aarch64
```

The `docker login` password is the personal access token from the one-time steps.

To push a trial tag, change the name after the colon (for example `manylinux_2_28_x86_64-test`). The wheel build keeps using the two tags in `pyproject.toml` until you push those names.

This figure is that laptop path: the Dockerfile, a maintainer machine, Docker Hub, then the Azure wheel jobs.

![](images/docker-workflow.png)

### Pull the image

```
docker pull neuronsimulator/neuron_wheel:manylinux_2_28_x86_64
```

### MPI support

The `neuronsimulator/neuron_wheel` provides out-of-the-box support for `mpich` and `openmpi`.
For `HPE-MPT MPI`, since it's not open source, they are provided automatically as part of Azure Pipelines and are not locally downloadable.

## macOS wheels

Note that for macOS there is no docker image needed, but all required dependencies must exist.
In order to have the wheels working on multiple macOS target versions, special consideration must be made for `MACOSX_DEPLOYMENT_TARGET`.

Taking Azure macOS `x86_64` wheels for example, `readline` was built with `MACOSX_DEPLOYMENT_TARGET=10.9` and stored as secure file on Azure (under `Pipelines > Library > Secure files`).
For `arm64` we need to set `MACOSX_DEPLOYMENT_TARGET=11.0`.

You can use [packaging/python/build_static_readline_osx.bash](../../packaging/python/build_static_readline_osx.bash) to build a static readline library.
You can have a look at the script for requirements and usage.

### Installing macOS prerequisites

Install the necessary Python versions by downloading the universal2 installers from https://www.python.org/downloads/macos/
You'll need several other packages installed as well (brew is fine):

```
brew install --cask xquartz
brew install flex bison mpich cmake
brew unlink mpich && brew install openmpi
brew uninstall --ignore-dependencies libomp || echo "libomp doesn't exist"
```

Bison and flex installed through brew will not be symlinked into /opt/homebrew (installing it next to the version provided by OSX can cause problems). To ensure the installed versions will actually be picked up:

```
export BREW_PREFIX=$(brew --prefix)
export PATH=/opt/homebrew/opt/bison/bin:/opt/homebrew/opt/flex/bin:$PATH
```

## Launch the wheel building

### Linux

You can build the wheel for a specific Python version using:
```
bash packaging/python/build_wheels.bash linux 39    # 39 for Python v3.9
```

To build wheels with CoreNEURON support you have to set the environmental variable `NRN_ENABLE_CORENEURON=ON`:
```
NRN_ENABLE_CORENEURON=ON bash packaging/python/build_wheels.bash linux '3*'
```
where we are passing `'3*'` (note the quotes!) to build the wheels with `CoreNEURON` support for all python 3 versions.

By default, the build system uses all of the processing units available on a machine; this can be customized using the `CMAKE_BUILD_PARALLEL_LEVEL` environmental variable.

Note that using [podman](https://podman.io/) is supported, however, you must set the environmental variable `CIBW_CONTAINER_ENGINE=podman` before launching the `build_wheels.bash` script.

### macOS
As mentioned above, for macOS all dependencies have to be available on a system. You have to then clone NEURON repository and execute:

```
cd nrn
bash packaging/python/build_wheels.bash osx 39  # 39 for Python v3.9
```

In some cases, setuptools-scm will see extra commits and consider your build as "dirty," resulting in filenames such as `NEURON-9.0a1.dev0+g9a96a3a4d.d20230717-cp310-cp310-macosx_11_0_arm64.whl` (which should have been `NEURON-9.0a0-cp310-cp310-macosx_11_0_arm64.whl`). If this happens, you can set an environment variable to correct this behavior:

```
export SETUPTOOLS_SCM_PRETEND_VERSION=9.0a
```

Change the pretend version to whatever is relevant for your case.

## Testing the wheels

To test the generated wheels, you can do:

```
# first arg is a python exe and second arg is the corresponding wheel
bash packaging/python/test_wheels.sh python3.9 wheelhouse/NEURON-7.8.0.236-cp39-cp39-macosx_10_9_x86_64.whl

# Or, you can provide the pypi url
bash packaging/python/test_wheels.sh python3.9 "-i https://test.pypi.org/simple/NEURON==7.8.11.2"
```

### MacOS considerations

On MacOS, launching `nrniv -python` or `special -python` can fail to load `neuron` module due to security restrictions.
For this specific purpose, please `export SKIP_EMBEDED_PYTHON_TEST=true` before launching the tests.

## Publishing the wheels on Pypi via Azure

### Variables that drive PyPI upload

We need to manipulate the following three predefined variables, listed hereafter with their default values:
   * `NRN_NIGHTLY_UPLOAD` : `true`
   * `NRN_RELEASE_UPLOAD` : `false`
   * `NEURON_NIGHTLY_TAG` : `-nightly`

### Release wheels

Head over to the [neuronsimulator.nrn](https://dev.azure.com/neuronsimulator/nrn/_build?definitionId=1) pipeline on Azure.

After creating the tag on the `release/x.y` or on the `master` branch, perform the following steps:

1) Click on `Run pipeline`
2) Input the release tag ref `refs/tags/x.y.z`
3) Click on `Advanced options` then select `Variables`
4) Update driving variables to:
   * `NRN_NIGHTLY_UPLOAD` : `false`
   * `NRN_RELEASE_UPLOAD` : `false`
   * `NEURON_NIGHTLY_TAG` : undefined (leave empty)

   Do so by clicking `Variables` in `Advanced options` and update/clear the variable values.
5) Click on `Run`

![](images/azure-release-no-upload.png)

With above, wheel will be created like release from the provided tag but they won't be uploaded to the pypi.org ( as we have set  `NRN_RELEASE_UPLOAD=false`). These wheels now you can download from artifacts section and perform thorough testing. Once you are happy with the testing result, set `NRN_RELEASE_UPLOAD` to `true` and trigger the pipeline same way:
   * `NRN_NIGHTLY_UPLOAD` : `false`
   * `NRN_RELEASE_UPLOAD` : `true`
   * `NEURON_NIGHTLY_TAG` : undefined (leave empty)



## Publishing the wheels on Pypi via CircleCI

Currently CircleCI doesn't have automated pipeline for uploading `release` wheels to pypi.org (nightly wheels are uploaded automatically though). Currently we are using a **hacky**, semi-automated approach described below:

* Checkout your tag as a new branch
* Update `.circleci/config.yml` as shown below
* Trigger CI pipeline manually for [the nrn project](https://app.circleci.com/pipelines/github/neuronsimulator/nrn)
* Upload wheels from artifacts manually

```
# checkout release tag as a new branch
$ git checkout 8.1a -b release/8.1a-aarch64

# manually updated `.circleci/config.yml`
$ git diff

@@ -14,6 +14,11 @@ jobs:

     machine:
       image: ubuntu-2004:202101-01
+    environment:
+      SETUPTOOLS_SCM_PRETEND_VERSION: 8.2.6
+      NEURON_NIGHTLY_TAG: ""
+      NRN_NIGHTLY_UPLOAD: false
+      NRN_RELEASE_UPLOAD: false

     resource_class: arm.medium

@@ -54,6 +59,7 @@ jobs:
               39) pyenv_py_ver="3.9.1" ;;
               310) pyenv_py_ver="3.10.1" ;;
               311) pyenv_py_ver="3.11.0" ;;
+              312) pyenv_py_ver="3.12.2" ;;
               *) echo "Error: pyenv python version not specified!" && exit 1;;
             esac

@@ -95,7 +101,7 @@ workflows:
                 - /circleci\/.*/
           matrix:
             parameters:
-              NRN_PYTHON_VERSION: ["311"]
+              NRN_PYTHON_VERSION: ["39", "310", "311", "312"]
               NRN_NIGHTLY_UPLOAD: ["false"]

   nightly:
```

The reason we are setting `SETUPTOOLS_SCM_PRETEND_VERSION` to a desired version `8.1a` because `pyproject.toml` uses `setuptools-scm` and it will give different version name as we are now on a new branch!
`SETUPTOOLS_SCM_PRETEND_VERSION` will also stop your wheels from getting extra numbers on the version.


## Nightly wheels

Nightly wheels get automatically published from `master` in CRON mode.


## How to test Azure wheels locally

After retrieving the Azure drop URL (i.e. from the GitHub PR comment, or by going to Azure for a specific build):

```bash
python3 -m pip wheel neuron-gpu-nightly --wheel-dir tmp --find-links 'https://dev.azure.com/neuronsimulator/aa1fb98d-a914-45c3-a215-5e5ef1bd7687/_apis/build/builds/7600/artifacts?artifactName=drop&api-version=7.0&%24format=zip'
```
will download the wheel and its dependencies to `tmp/` and then you can test it with:

```bash
./packaging/python/test_wheels.sh python3 ./tmp/NEURON_gpu_nightly-...whl true
```
