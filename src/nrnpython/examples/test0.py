from mpi4py import MPI
from neuron import h
import sys

pc = h.ParallelContext()

s = "mpi4py thinks I am %d of %d\
\nNEURON thinks I am %d of %d"

cw = MPI.COMM_WORLD
print s % (cw.rank, cw.size, \
           pc.id(),pc.nhost())

pc.done()

# Desired behaviour:

## emuller@drishti ~/hg/nrn_neurens_hg/src/nrnpython/examples $ mpiexec -np 4 nrniv -mpi -python test0.py
## numprocs=4
## NEURON -- VERSION 7.0.unoffical-nrn-hg.unknown (2216) 2008-09-15
## Duke, Yale, and the BlueBrain Project -- Copyright 1984-2008
## See http://www.neuron.yale.edu/credits.html

## MPI_Initialized==true, enabling MPI funtionality.
## MPI_Initialized==true, enabling MPI funtionality.
## MPI_Initialized==true, enabling MPI funtionality.
## MPI_Initialized==true, enabling MPI funtionality.
## numprocs=4
## mpi4py thinks I am 0 of 4, NEURON thinks I am 0 of 4
## mpi4py thinks I am 2 of 4, NEURON thinks I am 2 of 4

## mpi4py thinks I am 1 of 4, NEURON thinks I am 1 of 4
## mpi4py thinks I am 3 of 4, NEURON thinks I am 3 of 4



## >>> 
## emuller@drishti ~/hg/nrn_neurens_hg/src/nrnpython/examples $


## emuller@drishti ~/hg/nrn_neurens_hg/src/nrnpython/examples $ mpiexec -np 4 python test0.py
## MPI_Initialized==true, enabling MPI funtionality.
## MPI_Initialized==true, enabling MPI funtionality.
## MPI_Initialized==true, enabling MPI funtionality.
## MPI_Initialized==true, enabling MPI funtionality.
## numprocs=4
## NEURON -- VERSION 7.0.unoffical-nrn-hg.unknown (2216) 2008-09-15
## Duke, Yale, and the BlueBrain Project -- Copyright 1984-2008
## See http://www.neuron.yale.edu/credits.html

## mpi4py thinks I am 3 of 4, NEURON thinks I am 3 of 4

## mpi4py thinks I am 2 of 4, NEURON thinks I am 2 of 4

## mpi4py thinks I am 1 of 4, NEURON thinks I am 1 of 4

## mpi4py thinks I am 0 of 4, NEURON thinks I am 0 of 4

## emuller@drishti ~/hg/nrn_neurens_hg/src/nrnpython/examples $ python test0.py
## MPI_Initialized==true, enabling MPI funtionality.
## numprocs=1
## NEURON -- VERSION 7.0.unoffical-nrn-hg.unknown (2216) 2008-09-15
## Duke, Yale, and the BlueBrain Project -- Copyright 1984-2008
## See http://www.neuron.yale.edu/credits.html

## mpi4py thinks I am 0 of 1, NEURON thinks I am 0 of 1


## emuller@drishti ~/hg/nrn_neurens_hg/src/nrnpython/examples $ python
## Python 2.5.2 (r252:60911, Aug 22 2008, 18:45:05) 
## [GCC 4.1.2 (Gentoo 4.1.2 p1.1)] on linux2
## Type "help", "copyright", "credits" or "license" for more information.
## >>> import neuron
## MPI_Initialized==false, disabling MPI funtionality.
## NEURON -- VERSION 7.0.unoffical-nrn-hg.unknown (2216) 2008-09-15
## Duke, Yale, and the BlueBrain Project -- Copyright 1984-2008
## See http://www.neuron.yale.edu/credits.html

## >>>

