# Building Python Wheels

See also [this document](../dev/python/wheels).

## Linux wheels

In order to have NEURON binaries run on most Linux distros, we rely on the [manylinux project](https://github.com/pypa/manylinux).
Current NEURON Linux image is based on `manylinux_2_28`.

### Setting up Docker

[Docker](https://en.wikipedia.org/wiki/Docker_(software)) is required for building Linux wheels.
You can find instructions on how to setup Docker on Linux [here](https://docs.docker.com/engine/install/).


### NEURON Docker Image Workflow

When required (i.e. update packages, add new software), `NEURON maintainers` are in charge of
updating the NEURON docker images published on Docker Hub under
[neuronsimulator/neuron_wheel](https://hub.docker.com/r/neuronsimulator/neuron_wheel).

GitHub Actions (and the remaining Azure pipeline) pull this image off DockerHub for Linux wheels building.

Updating and publishing the public images are done by a manual process that relies on a
`Docker file`  (see [packaging/python/Dockerfile](../../packaging/python/Dockerfile)).
Any official update of these files shall imply a PR reviewed and merged before `DockerHub` publishing.

Release and nightly wheels are published from **GitHub Actions**:

* `neuron-nightly` from the [nightly wheels workflow](https://github.com/neuronsimulator/nrn/actions/workflows/wheels-nightly.yml)
* `neuron-x.y.z` from the [NEURON Release](https://github.com/neuronsimulator/nrn/actions/workflows/release.yml) workflow (`upload=true`)
* Merged as a GHA artifact named `wheels` on each of those runs

An Azure pipeline still builds some PR/nightly wheels and stores them as Azure `drop` artifacts; it is **not** the release publish path.

Refer to the following image for the NEURON Docker Image workflow:
![](images/docker-workflow.png)


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

### MPI support

The `neuronsimulator/neuron_wheel` provides out-of-the-box support for `mpich` and `openmpi`.
For `HPE-MPT MPI`, since it's not open source, they are provided automatically as part of Azure Pipelines and are not locally downloadable.

### CI dependency archive (pinned downloads)

Wheel **test** jobs install both MPICH and OpenMPI so
[packaging/python/test_wheels.sh](../../packaging/python/test_wheels.sh) can
exercise dynamic MPI. On Ubuntu 24.04, stock MPICH was broken
([LP#2072338](https://bugs.launchpad.net/ubuntu/+source/mpich/+bug/2072338));
CI therefore installs a **pinned** pair of `.deb` files from the dedicated
CI-deps archive
[nrn-ci-deps / ci-deps-v1](https://github.com/neuronsimulator/nrn-ci-deps/releases/tag/ci-deps-v1)
instead of downloading them from Launchpad on every run (and instead of mixing
pins into NEURON product Releases on `nrn`).

See **[CI dependency archive](ci_deps.md)** and
[ci/deps/README.md](../../ci/deps/README.md) for:

* the catalog (`MANIFEST.yml`) and `managed: true|false`
* hosting on [neuronsimulator/nrn-ci-deps](https://github.com/neuronsimulator/nrn-ci-deps)
* `fetch.sh` / `install_mpich_noble.sh` / `publish.sh` / `check-upstream.sh`
* how to add the next flaky third-party download

Azure macOS wheels still obtain a prebuilt static **readline** via an Azure
*secure file* (see macOS section below). Migrating that class of blob into
`ci/deps` is tracked as `managed: false` in the MANIFEST until promoted.

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

## Publishing the wheels on Pypi via GitHub Actions

Release wheels are built and published by the
[NEURON Release](https://github.com/neuronsimulator/nrn/actions/workflows/release.yml)
workflow, not Azure.

### Three knobs

When you click **Run workflow**:

* **Use workflow from** (controller) — which checkout provides `release.yml`. Use **`master`** for dry-run and ship.
* **`rel_branch`** — the git ref whose sources are built (usually `release/x.y`).
* **`rel_tag`** — the version name (`x.y.z`).

`upload` is a fourth input: `false` = dry-run, `true` = create the tag, attach artifacts to a GitHub pre-release, and publish wheels to PyPI.

### Release wheels (dry-run, then ship)

1. Open [NEURON Release](https://github.com/neuronsimulator/nrn/actions/workflows/release.yml) → **Run workflow**.
2. **Use workflow from:** `master`.
3. Set `rel_branch` to `release/x.y` (or the cherry-pick branch for a pre-merge smoke).
4. Set `rel_tag` to `x.y.z`.
5. Leave Python/OS lists at the defaults unless you are deliberately narrowing the matrix.
6. Set **`upload` to `false`** and run. This builds and tests wheels, runs ModelDB CI and nrn-build-ci against **this run’s** merged `wheels` artifact, and builds the full-src package and Windows installer. It does **not** push a tag or upload to PyPI.
7. Confirm `neuron.__version__` is non-empty on a wheel from the artifact, and that ModelDB V2 used this run’s artifact URL (not a nightly fallback).
8. If only ModelDB failed, retest without rebuilding wheels: [ModelDB CI (reuse wheels)](https://github.com/neuronsimulator/nrn/actions/workflows/modeldb-ci-reuse-wheels.yml). Pass the dry-run `wheels` artifact id or `https://github.com/neuronsimulator/nrn/actions/artifacts/<id>` URL, `neuron_v1=neuron==<previous>`, and `modeldb_ci_ref=master`.
9. When the dry-run is green **and** `rel_branch` still points at the same SHA, run the same workflow again with **`upload=true`**.

Do not treat Azure `NRN_RELEASE_UPLOAD` as the release path.

### Nightly wheels

Nightly wheels are published from `master` by
[wheels-nightly.yml](https://github.com/neuronsimulator/nrn/actions/workflows/wheels-nightly.yml)
on a schedule (and can be dispatched manually).

## Publishing the wheels on Pypi via CircleCI

Linux/arm64 release wheels are now part of the GHA matrix (`ubuntu-24.04-arm`). There is no `.circleci` config in this repository; do not use CircleCI for a release.

## How to test GHA wheels locally

Download the merged `wheels` artifact from a [NEURON Release](https://github.com/neuronsimulator/nrn/actions/workflows/release.yml) or [wheels-ci](https://github.com/neuronsimulator/nrn/actions/workflows/wheels-ci.yml) run, unzip it, and pass a `.whl` to `packaging/python/test_wheels.sh`.

## How to test Azure wheels locally

Azure still publishes a `drop` zip for some PR/nightly builds. After retrieving the Azure drop URL (i.e. from the GitHub PR comment, or by going to Azure for a specific build):

```bash
python3 -m pip wheel neuron-gpu-nightly --wheel-dir tmp --find-links 'https://dev.azure.com/neuronsimulator/aa1fb98d-a914-45c3-a215-5e5ef1bd7687/_apis/build/builds/7600/artifacts?artifactName=drop&api-version=7.0&%24format=zip'
```
will download the wheel and its dependencies to `tmp/` and then you can test it with:

```bash
./packaging/python/test_wheels.sh python3 ./tmp/NEURON_gpu_nightly-...whl true
```
