
# Building Python Wheels

## Linux wheels

In order to have NEURON binaries run on most Linux distros, we rely on the [manylinux project](https://github.com/pypa/manylinux).
Current NEURON Linux image is based on `manylinux2014`.

### Setting up Docker

[Docker](https://en.wikipedia.org/wiki/Docker_(software)) is required for building Linux wheels.
You can find instructions on how to setup Docker on Linux [here](https://docs.docker.com/engine/install/). 


### NEURON Docker Image Workflow

When required (i.e. update packages, add new software), `NEURON maintainers` are in charge of updating the NEURON docker
image published on Docker Hub under [neuronsimulator/neuron_wheel](https://hub.docker.com/r/neuronsimulator/neuron_wheel).
Azure pipelines pull this image off DockerHub for Linux wheels building.

Updating and publishing the public image is a manual process that relies on a `Dockerfile` (see [packaging/python/Dockerfile](../../packaging/python/Dockerfile)).
Any official update of this file shall imply a PR reviewed and merged before `DockerHub` publishing.

All wheels built on Azure (also including macOS) are:

* Published to `Pypi.org` as
  * `neuron_nightly` -> when the pipeline is launched in CRON mode
  * `neuron-x.y.z` -> when the pipeline is manually triggered for release `x.y.z`
* Stored as `Azure artifacts` in the Azure pipeline for every run.

Refer to the following image for the NEURON Docker Image workflow: 
![](images/docker-workflow.png)


### Building the docker image
After making updates to the [packaging/python/Dockerfile](../../packaging/python/Dockerfile), you can build it with:
```
cd nrn/packaging/python
# update Dockerfile
docker build -t neuronsimulator/neuron_wheel:<tag> .
```
where `<tag>` is:
* `latest` for official publishing (after merging related PR)
* `feature-name` for updates (for local testing or for PR testing purposes where you can temporarily publish the tag on DockerHub and tweak Azure CI pipelines to use it - refer to `Job: 'ManyLinuxWheels'` in [azure-pipelines.yml](../../azure-pipelines.yml) )

### Pushing to DockerHub

In order to push the image and its tag:
```
docker login --username=<username>
docker push neuronsimulator/neuron_wheel:<tag>
```

### Using the docker image

You can either build the neuron image locally or pull the image from DockerHub:
```
$ docker pull neuronsimulator/neuron_wheel
Using default tag: latest
latest: Pulling from neuronsimulator/neuron_wheel
....
Status: Downloaded newer image for neuronsimulator/neuron_wheel:latest
docker.io/neuronsimulator/neuron_wheel:latest
```

We can conveniently mount the local NEURON repository inside docker, by using the `-v` option:

```
docker run -v /home/user/nrn:/root/nrn -it neuronsimulator/neuron_wheel bash
```
where `/home/user/nrn` is a NEURON repository on the host machine that ends up mounted at `/root/nrn`.
This is how you can test your NEURON updates inside the NEURON Docker image.

### MPI support

The `neuronsimulator/neuron_wheel` provides out-of-the-box support for `mpich` and `openmpi`.
For `HPE MPT`, since it's not open source, you need to acquire the headers and mount them in the docker image:

```
docker run -v /home/user/nrn:/root/nrn -v /home/user/mpt-headers:/nrnwheel/mpt -it neuronsimulator/neuron_wheel bash
```
where `/home/user/mpt-headers` is the path to the MPT headers on the host machine that end up mounted at `/nrnwheel/mpt`.
You can get the headers with:

```
git clone ssh://bbpcode.epfl.ch/user/kumbhar/mpt-headers
```

## macOS wheels
Note that for macOS there is no docker image needed, but all required dependencies must exist.
In order to have the wheels working on multiple macOS target versions, special consideration must be made for MACOSX_DEPLOYMENT_TARGET.

Taking Azure macOS wheels for example, `readline` has been manually built with `MACOSX_DEPLOYMENT_TARGET=10.9` and stored as secure file on Azure.
See [packaging/python/Dockerfile](../../packaging/python/Dockerfile) for readline build instructions.

## Launch the wheel building

### Linux
Once we've cloned and mounted NEURON inside Docker(c.f. `-v` option described previously), we can proceed with wheels building. 
There is a build script which loops over available pythons in the Docker image under `/opt/python`, and then builds and audits the generated wheels.
Wheels are generated under `/root/nrn/wheelhouse` and also accessible in the mounted NEURON folder from outside the Docker image.

```
cd /root/nrn
bash packaging/python/build_wheels.bash linux 
ls -la wheelhouse
```

You can build the wheel for a specific python version: 
```
bash packaging/python/build_wheels.bash linux 3.8
```

### macOS
As mentioned above, for macOS all dependencies have to be available on a system. You have to then clone NEURON repository and execute:

```
cd nrn
bash packaging/python/build_wheels.bash osx
```

## Testing the wheels

To test the generated wheels, you can do:

```
# first arg is a python exe and second arg is the corresponding wheel
bash packaging/python/test_wheels.sh python3.8 wheelhouse/NEURON-7.8.0.236-cp38-cp38-macosx_10_9_x86_64.whl
```
