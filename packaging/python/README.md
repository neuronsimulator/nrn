
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
docker run -v /home/user/nrn:/root/nrn -v /home/user/mpt-headers/2.21:/nrnwheel/mpt -it neuronsimulator/neuron_wheel bash
```

where `/home/user/nrn` is a neuron repository on the host machine and `/home/user/mpt` is a directory containing MPT MPI headers. We mount those directories inside docker at location `/root/nrn` and `/nrnwheel/mpt` inside the container. The MPT headers in the separate repository as it's not open source library. You can download the headers as:

```
git clone ssh://bbpcode.epfl.ch/user/kumbhar/mpt-headers
```

Note that for OS X there is no docker image but on a system where all dependencies exist, you have to perform next building step.

#### Launch the wheel building
Once we are inside docker container, we can start building wheels. There is a build script which loop over the pythons `>=3.5` in `/opt/python`, build and audit the generated wheels. Results are placed in this wheelhouse directory.

```
cd /root/nrn
bash packaging/python/build_wheels.bash linux
```

For OSX on a system with the all dependencies you have to clone NEURON repository and have to do:

```
cd nrn
bash packaging/python/build_wheels.bash osx
```

#### Testing wheels

To test the generated wheels, you can do:

```
# first arg as python exe and second arg as a corresponding wheel
bash packaging/python/test_wheels.sh python3.8 wheelhouse/NEURON-7.8.0.236-cp38-cp38-macosx_10_9_x86_64.whl

# Or, you can provide pypi url as
bash packaging/python/test_wheels.sh python3.8 "-i https://test.pypi.org/simple/ NEURON==7.8.11.2"
```

On BB5, we test wheels as:

```
salloc -A proj16  -N 1 --ntasks-per-node=4 -C "cpu" --time=1:00:00 -p interactive
module load unstable python/3.7.4
bash packaging/python/test_wheels.sh python3.7 wheelhouse/NEURON-7.8.0.236-cp37-cp37m-manylinux1_x86_64.whl
```

#### Upload wheels

As we haven't setup proper policy yet, do not do this step. For testing purpose you can upload the wheels on [test.pypi.org/project/NEURON](https://test.pypi.org/project/NEURON/) as:

```
pip3 install twine
python3 -m twine upload --repository-url https://test.pypi.org/legacy/ nrn/wheelhouse/NEURON-*
```

#### Updating neuron_wheel docker image

If you have changed Dockerfile, you can build the new image as:

```
docker build -t neuronsimulator/neuron_wheel .
```

and then push image to hub.docker.com as:

```
docker login --username=<username>
docker push neuronsimulator/neuron_wheel
```
