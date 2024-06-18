
# Building Python Wheels

## Linux wheels

In order to have NEURON binaries run on most Linux distros, we rely on the [manylinux project](https://github.com/pypa/manylinux).
Current NEURON Linux image is based on `manylinux2014`.

### Setting up Docker

[Docker](https://en.wikipedia.org/wiki/Docker_(software)) is required for building Linux wheels.
You can find instructions on how to setup Docker on Linux [here](https://docs.docker.com/engine/install/).


### NEURON Docker Image Workflow

When required (i.e. update packages, add new software), `NEURON maintainers` are in charge of
updating the NEURON docker images published on Docker Hub under
[neuronsimulator/neuron_wheel](https://hub.docker.com/r/neuronsimulator/neuron_wheel).

Azure pipelines pull this image off DockerHub for Linux wheels building.

Updating and publishing the public images are done by a manual process that relies on a
`Docker file`  (see [packaging/python/Dockerfile](../../packaging/python/Dockerfile)).
Any official update of these files shall imply a PR reviewed and merged before `DockerHub` publishing.

All wheels built on Azure are:

* Published to `Pypi.org` as
  * `neuron-nightly` -> when the pipeline is launched in CRON mode
  * `neuron-x.y.z` -> when the pipeline is manually triggered for release `x.y.z`
* Stored as `Azure artifacts` in the Azure pipeline for every run.

Refer to the following image for the NEURON Docker Image workflow:
![](images/docker-workflow.png)


### Building the docker images automatically
If you run the workflow manually on Gitlab (with the "Run pipeline" button), it will now have the `mac_m1_container_build` and `x86_64_container_build` jobs added to it. These jobs need to be started manually and will not affect the overal workflow status. They don't need to be run every time, just when a refresh of the container images is necessary.
They will build the container images and push to docker hub. If you want to, you can still build manually (see next section), but there shouldn't be a requirement to do so any more.

A word of warning: podman on OSX uses a virtual machine. The job can take care of starting it, but we generally try to have it running to avoid jobs cleaning up after themselves and killing the machine for other jobs. When starting the machine, set the variables that need to be set during the container build, ie. proxy and `BUILDAH_FORMAT`.

`BUILDAH_FORMAT` ensures that `ONBUILD` instructions are enabled.

```
export http_proxy=http://bbpproxy.epfl.ch:80
export https_proxy=http://bbpproxy.epfl.ch:80
export HTTP_PROXY=http://bbpproxy.epfl.ch:80
export HTTPS_PROXY=http://bbpproxy.epfl.ch:80
export BUILDAH_FORMAT=docker
```

### Building the docker image manually
After making updates to any of the docker files, you can build the image with:
```
cd nrn/packaging/python
# update Dockerfile
docker build -t neuronsimulator/neuron_wheel:<tag> .
```
where `<tag>` is:
* `latest-x86_64` or `latest-aarch64` for official publishing on respective platforms. For `master`, we are using `latest-gcc9-x86_64` and `latest-gcc9-aarch64` (see [Use GCC9 for building wheels #1971](https://github.com/neuronsimulator/nrn/pull/1971)).
* `feature-name` for updates (for local testing or for PR testing purposes where you can temporarily publish the tag on DockerHub and tweak Azure CI pipelines to use it - refer to
  `Job: 'ManyLinuxWheels'` in [azure-pipelines.yml](../../azure-pipelines.yml) )

If you are building an image for AArch64 i.e. with `latest-aarch64` tag then you additionally pass `--build-arg` argument to docker build command in order to use compatible manylinux image for ARM64 platform (e.g. while building on Apple M1 or QEMU emulation):

```
docker build -t neuronsimulator/neuron_wheel:latest-aarch64 --build-arg MANYLINUX_IMAGE=manylinux2014_aarch64 -f Dockerfile .
```


### Pushing to DockerHub

In order to push the image and its tag:
```
docker login --username=<username>
docker push neuronsimulator/neuron_wheel:<tag>
```

### Using the docker image

You can either build the neuron images locally or pull them from DockerHub:
```
$ docker pull neuronsimulator/neuron_wheel:latest-x86_64
Using default tag: latest-x86_64
latest: Pulling from neuronsimulator/neuron_wheel
....
Status: Downloaded newer image for neuronsimulator/neuron_wheel:latest
docker.io/neuronsimulator/neuron_wheel:latest-x86_64
```

We can conveniently mount the local NEURON repository inside docker, by using the `-v` option:

```
docker run -v $PWD/nrn:/root/nrn -w /root/nrn -it neuronsimulator/neuron_wheel:latest-x86_64 bash
```
where `$PWD/nrn` is a NEURON repository on the host machine that ends up mounted at `/root/nrn`.
This is how you can test your NEURON updates inside the NEURON Docker image.
Note that `-w` sets the working directory inside the container.

### MPI support

The `neuronsimulator/neuron_wheel` provides out-of-the-box support for `mpich` and `openmpi`.
For `HPE-MPT MPI`, since it's not open source, you need to acquire the headers and mount them in the docker image:

```
docker run -v $PWD/nrn:/root/nrn -w /root/nrn -v $PWD/mpt-headers/2.21/include:/nrnwheel/mpt/include -it neuronsimulator/neuron_wheel:latest-x86_64 bash
```
where `$PWD/mpt-headers` is the path to the HPE-MPT MPI headers on the host machine that end up mounted at `/nrnwheel/mpt/include`.
You can download the headers with:

```
git clone ssh://bbpcode.epfl.ch/user/kumbhar/mpt-headers
```

## macOS wheels

Note that for macOS there is no docker image needed, but all required dependencies must exist.
In order to have the wheels working on multiple macOS target versions, special consideration must be made for `MACOSX_DEPLOYMENT_TARGET`.


Taking Azure macOS `x86_64` wheels for example, `readline` was built with `MACOSX_DEPLOYMENT_TARGET=10.9` and stored as secure file on Azure (under `Pipelines > Library > Secure files`).
For `arm64` we need to set `MACOSX_DEPLOYMENT_TARGET=11.0`. The wheels currently need to be built manually, using `universal2` Python installers.
For upcoming `universal2` wheels (targeting both `x86_64` and `arm64`) we will consider leveling everything to `MACOSX_DEPLOYMENT_TARGET=11.0`.


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
Once we've cloned and mounted NEURON inside Docker(c.f. `-v` option described previously), we can proceed with wheels building.
There is a build script which loops over available pythons in the Docker image under `/opt/python`, and then builds and audits the generated wheels.
Wheels are generated under `/root/nrn/wheelhouse` and also accessible in the mounted NEURON folder from outside the Docker image.

```
# Working directory is /root/nrn
bash packaging/python/build_wheels.bash linux
ls -la wheelhouse
```

You can build the wheel for a specific python version:
```
bash packaging/python/build_wheels.bash linux 38    # 38 for Python v3.8
```

To build wheels with CoreNEURON support you have to pass an additional argument: `coreneuron`.
```
bash packaging/python/build_wheels.bash linux 3* coreneuron
```
Where we are passing `3*` to build the wheels with `CoreNEURON` support for all python 3 versions.

### macOS
As mentioned above, for macOS all dependencies have to be available on a system. You have to then clone NEURON repository and execute:

```
cd nrn
bash packaging/python/build_wheels.bash osx
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
bash packaging/python/test_wheels.sh python3.8 wheelhouse/NEURON-7.8.0.236-cp38-cp38-macosx_10_9_x86_64.whl

# Or, you can provide the pypi url
bash packaging/python/test_wheels.sh python3.8 "-i https://test.pypi.org/simple/NEURON==7.8.11.2"
```

### MacOS considerations

On MacOS, launching `nrniv -python` or `special -python` can fail to load `neuron` module due to security restrictions.
For this specific purpose, please `export SKIP_EMBEDED_PYTHON_TEST=true` before launching the tests.

### Testing on BB5
On BB5, we can test CPU wheels with:

```
salloc -A proj16  -N 1 --ntasks-per-node=4 -C "cpu" --time=1:00:00 -p interactive
module load unstable python
bash packaging/python/test_wheels.sh python3.8 wheelhouse/NEURON-7.8.0.236-cp38-cp38m-manylinux1_x86_64.whl
```

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

@@ -15,6 +15,10 @@ jobs:
     machine:
       image: ubuntu-2004:202101-01
+    environment:
+      SETUPTOOLS_SCM_PRETEND_VERSION: 8.1a
+      NEURON_NIGHTLY_TAG: ""
+      NRN_NIGHTLY_UPLOAD: false
+      NRN_RELEASE_UPLOAD: false

@@ -89,7 +95,7 @@ workflows:
       - manylinux2014-aarch64:
           matrix:
             parameters:
-              NRN_PYTHON_VERSION: ["311"]
+              NRN_PYTHON_VERSION: ["38", "39", "310", "311"]
```

The reason we are setting `SETUPTOOLS_SCM_PRETEND_VERSION` to a desired version `8.1a` because `setup.py` uses `git describe` and it will give different version name as we are now on a new branch!


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
