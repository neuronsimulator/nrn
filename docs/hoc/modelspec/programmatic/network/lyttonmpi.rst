
.. _hoc_lytton_mpi:

% $Id: lyttonmpi.txt,v 1.1 2005/08/23 15:47:59 hines Exp $

* MPICH can be obtained from 
  http://www-unix.mcs.anl.gov/mpi/mpich/downloads/mpich.tar.gz 
  (tested Jul 30, 2005)

MPICH must be compiled for every machine type that you want to use
in your parallel environment.

NEURON must be built and accessible on every individual cpu (node)
on which you wish to run in parallel

* Make sure that you can readily log into yourself (or other nodes)
  without a password.
  I will assume in what follows that you are using ssh to access machines.
  To test access to your local machine use 'ssh localhost'.
  If you require a password to login you may want to use RSA??

* By default MPICH assumes that you will be using rsh to login to remote nodes.
  This is unlikelyto be the case on a current machine.  Therefore, the environmental
  variable RSHCOMMAND mush be set BEFORE compile MPICH.  This can be done
  as follows.

      export RSHCOMMAND="ssh" # sh/bash
      setenv RSHCOMMAND "ssh" # csh

* We assume that you are using a machine that utilizes TCP/IP for communication.
  This being the case you will use the following command to configure and compile:
  
      ./configure --with-device=ch_p4 --prefix=WHERE_YOU_WANT_IT
      make
      make install

* After apparently successful installation you can test the installation
  as follows.

      cd examples/basic/
      make
      mpirun -np 2 hello++
      
  This should return messages from 2 nodes (or from the same CPU if only 1 CPU)

      Hello World! I am 0 of 2
      Hello World! I am 1 of 2

* Before continuing with the NEURON compilation make sure that the paths
  are set correctly on the machine, ie for bash

    export MPICH= ...   # eg /home/you/mpich-1.2.7/i686
    export MPICH_PATH="${MPICH}/bin"
    export MPICH_LIB="${MPICH}/lib"
    export PATH="${MPICH_PATH}:${PATH}"
    export LD_LIBRARY_PATH="${MPICH_LIB}:${LD_LIBRARY_PATH}"

* NEURON compilation is done using the --with-mpi flag to configure
  ./configure --prefix=WHERE_TO --with-iv=WHERE_IS_IV --with-mpi
  make
  make install

* to test the parallel NEURON 

      cd src/parallel
      mpirun -np 2 /home/billl/nrniv/nrn-5.8.39/x86_64/bin/nrniv test0.hoc
      
  which should return   
  
      hello from id 0 on YOUR_MACHINE
      
  hello from id 1 on YOUR_MACHINE


