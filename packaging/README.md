
### Building Python Wheels

Note: This is only slightly adapted from NEURONs [scripts](https://github.com/neuronsimulator/nrn/tree/master/packaging/python).

NMODL wheels are built in a manylinux2010 image. Since the generic docker image is very basic (CentOS 6) a new image, which brings updated cmake3 (3.12), flex and bison was prepared and made available at https://hub.docker.com/r/bluebrain/nmodl (tag: wheel).

#### Setting up Docker

[Docker](https://en.wikipedia.org/wiki/Docker_(software)) is required for building Linux wheels. You can find instructions to setup Docker on Linux [here](https://docs.docker.com/engine/install/ubuntu/) and on OS X [here](https://docs.docker.com/docker-for-mac/install/). On Ubuntu system we typically do:

```
sudo apt install docker.io
sudo groupadd docker
sudo usermod -aG docker $USER
```

Logout and log back in to have docker service properly configured.

#### Pull and start the docker image

We mount local neuron repository inside docker as a volume to preserve any code changed. We can use -v option to mount the local folder as:

```
docker run -v /home/user/nmodl:/root/nmodl -it bluebrain/nmodl:wheel bash
```

where `/home/user/nmodl` is the NMODL repository on the host machine. We mount this directory inside docker at location `/root/nmodl` inside the container.

Note that for OS X there is no docker image but on a system where all dependencies exist, you have to perform next building step.

#### Launch the wheel building
Once we are inside docker container, we can start building wheels. There is a build script that loops over the pythons `>=3.5` in `/opt/python`, build and audit the generated wheels. Results are placed in this wheelhouse directory.

```
cd /root/nmodl
bash packaging/python/build_wheels.bash linux
```

For OSX on a system with the all dependencies you have to clone the NMODL repository and do:

```
cd nrn
bash packaging/python/build_wheels.bash osx
```

#### Updating neuron_wheel docker image

If you have changed Dockerfile, you can build the new image as:

```
docker build -t bluebrain/nmodl:tag .
```

and then push image to hub.docker.com as:

```
docker login --username=<username>
docker push bluebrain/nmodl:tag 
```
