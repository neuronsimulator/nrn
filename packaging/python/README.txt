Building Python Wheels
======================

For the highest compatibility, NEURON wheels are built in a
manylinux1 image. Since the generic docker image is very basic
(centos5) a new image, which brings updated libssl, cmake3 (3.12),
ncurses, openmpi and mpich was prepared and made available at
docker.io/pkumbhar/neuron_wheel

Steps:

1. Pull and start the docker image
   -------------------------------
We mount local neuron repository inside docker as a volume to preserve
any code changed. We can use -v option to mount the local folder as:

$ docker run -v /home/user/nrn:/root/nrn -it docker.io/pkumbhar/neuron_wheel bash

where `/home/user/nrn` is a neuron repository on host machine and we mount
that inside docker at location /root/nrn.

TODO : Add instructions for OS X.
  - Note that for OS X there is no docker image but on a system where all
    dependencies exist, you have to just launch build_wheels.osx.bash.


2. Launch the wheel building
   -------------------------

Once we are inside docker container, we can start building wheels.
There is a build script which loop over the pythons >=3.5 in /opt/python,
build and audit the generated wheels. Results are placed in this wheelhouse
directory:

$ cd /root
$ bash nrn/build_wheels.linux.bash


3. Upload wheels
   -------------------------

As we haven't setup proper policy yet, do not do this step.
Note that wheels are built with root user. You might want to
change owner and group with chown.
