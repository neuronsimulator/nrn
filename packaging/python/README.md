
### Building Python Wheels

For the highest compatibility, NEURON wheels are built in a manylinux1 image. Since the generic docker image is very basic (centos5) a new image, which brings updated libssl, cmake3 (3.12), ncurses, openmpi and mpich was prepared and made available at https://hub.docker.com/u/neuronsimulator.

#### Setting up Docker

[Docker](https://en.wikipedia.org/wiki/Docker_(software)) is required for building Linux wheels. You can instructions to
setup Docker on Linux [here](https://docs.docker.com/engine/install/ubuntu/) and on OS X
[here](https://docs.docker.com/docker-for-mac/install/). On Ubuntu system we typically do:

```
sudo apt install docker.io
sudo groupadd docker
sudo usermod -aG docker $USER
```

Logout and log back in to have docker service properly configured.

#### Pull and start the docker image

We mount local neuron repository inside docker as a volume to preserve any code changed. We can use -v option to mount the local folder as:

```
git clone https://github.com/neuronsimulator/nrn.git

docker run -w /root/nrn -v $PWD/nrn:/root/nrn -v $PWD/mpt-headers/2.21/include:/nrnwheel/mpt/include -it neuronsimulator/neuron_wheel bash
```

where `$PWD/nrn` is a neuron repository on the host machine and `$PWD/mpt-headers` is a directory containing HPE-MPT MPI headers (optional). We mount those directories inside docker at location `/root/nrn` and `/nrnwheel/mpt/include` inside the container. The MPT headers are optional and maintained in the separate repository as it's not open source library. You can download the headers as:

```
git clone ssh://bbpcode.epfl.ch/user/kumbhar/mpt-headers
```

If you want to build wheel with *GPU support* via CoreNEURON then we have to use image `neuronsimulator/neuron_wheel_gpu` i.e.

```
docker run -w /root/nrn -v $PWD/nrn:/root/nrn -v $PWD/mpt-headers/2.21/include:/nrnwheel/mpt/include -it neuronsimulator/neuron_wheel_gpu bash
```

Note that for OS X there is no docker image but on a system where all dependencies exist, you have to perform next building step.

#### Launch the wheel building
Once we are inside docker container, we can start building wheels. There is a build script which loop over the pythons `>=3.5` in `/opt/python`, build and audit the generated wheels. Results are placed in this wheelhouse directory.

```
bash packaging/python/build_wheels.bash linux
```

You can build wheel for a specific python version by specifing version as:

```
bash packaging/python/build_wheels.bash linux 37        # 37 for Python v3.7
```

To build wheel with GPU support you have to pass an additional argument:
* coreneuron : build wheel with coreneuron support
* coreneuron-gpu : build wheel with coreneuron and gpu support

```
bash packaging/python/build_wheels.bash linux 3* coreneuron
or
bash packaging/python/build_wheels.bash linux 38 coreneuron-gpu
```

In the above example we are passing `3*` to build the wheels for all python 3 version.

For OSX on a system with the all dependencies you have to clone NEURON repository and have to do:

```
cd nrn
bash packaging/python/build_wheels.bash osx
```

#### Testing wheels

To test the generated wheels, you can execute following commands from nrn repository:

```
# first arg as python exe and second arg as a corresponding wheel
bash packaging/python/test_wheels.sh python3.8 wheelhouse/NEURON-7.8.0.236-cp38-cp38-macosx_10_9_x86_64.whl

# Or, you can provide pypi url as
bash packaging/python/test_wheels.sh python3.8 "-i https://test.pypi.org/simple/ NEURON==7.8.11.2"
```

On BB5, we test CPU wheels as:

```
salloc -A proj16  -N 1 --ntasks-per-node=4 -C "cpu" --time=1:00:00 -p interactive
module load unstable python
bash packaging/python/test_wheels.sh python3.7 wheelhouse/NEURON-7.8.0.236-cp37-cp37m-manylinux1_x86_64.whl
```

The GPU wheel can be also tested in same way on the CPU partition. In this case only pre-compiled binaries
like nrniv and nrniv-core are tested on CPU. In order to test full functionality of GPU wheel we need to
do following:
* Allocate GPU node
* Load NVHPC compiler
* Launch `test_wheels.sh`

```
salloc -A proj16 -N 1 --ntasks-per-node=4 -C "volta" --time=1:00:00 -p prod --partition=prod --exclusive
module load unstable python nvhpc

bash packaging/python/test_wheels.sh python3 NEURON_gpu_nightly-8.0a709-cp38-cp38-manylinux_2_17_x86_64.manylinux2014_x86_64.whl
```

The `test_wheels.sh` will check if nvc/nvc++ compiler is available and run tests with hpe-mpi, intel-mpi and mvapich2 MPI modules.
Also, it checks if GPU is available (using `pgaccelinfo -nvidia` command) and then run few tests on GPU as well.


#### Upload wheels

As we haven't setup proper policy yet, do not do this step. For testing purpose you can upload the wheels on [test.pypi.org/project/NEURON](https://test.pypi.org/project/NEURON/) as:

```
pip3 install twine
python3 -m twine upload --repository-url https://test.pypi.org/legacy/ nrn/wheelhouse/NEURON-*
```

#### Updating neuron_wheel docker image

If you have changed Dockerfile, you can build the new image under `packaging/python` as:

```
docker build -t neuronsimulator/neuron_wheel .
```

and then push image to hub.docker.com as:

```
docker login --username=<username>
docker push neuronsimulator/neuron_wheel
```

Note that we have a separate dockerfile `Dockerfile_gpu` to build wheels with GPU support via CoreNEURON.
In order to build or update image then do:

```
docker build -t neuronsimulator/neuron_wheel_gpu -f Dockerfile_gpu .
```
