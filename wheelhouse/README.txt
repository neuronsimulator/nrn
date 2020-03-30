Building Python Wheels
======================

For the highest compatibility, Neuron wheels are built in a
manylinux1 image. Since the generic docker image is very basic (centos5)
a new image, which brings updated libssl, cmake3 (3.12), etc was 
prepared and made available at 
docker.io/ferdonline/manylinux1_steroids_x86_64

Steps:

1. Pull and start the docker image
   -------------------------------
Non-image devel files can go into its separate volume, so that changes
are preserved across runs. e.g.
$ docker run -v nrn_dev:/root/dev/neuron -it ferdonline/manylinux1_steroids_x86_64 bash

where nrn_dev is a simple docker volume created with `docker volume create nrn_dev``


2. Install system requirements (flex)
   ----------------------------------
yum install -Y flex


3. Clone neuron
   ------------
For the moment clone the python_wheel branch. Save time/space with `--depth=N`

$ cd dev/neuron
$ git clone https://github.com/neuronsimulator/nrn.git -b python_wheel --depth=2


4. Launch the wheel building
   -------------------------

There is a build script (to be launched from the outer dir)
It will loop over the pythons >=3.5 in /opt/python, build and audit
the generated wheels. Results are placed in this directory

$ ln -s nrn/build_wheels.bash
$ bash build_wheels.bash

