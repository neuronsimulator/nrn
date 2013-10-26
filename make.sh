nrnmpiSRC='nrnmpi mpispike'

nrnocSRC='capac eion finitialize fadvance_core solve_core treeset_core nrnoc_aux'

nrnivSRC='netpar netcvode cxprop cvodestb tqueue htlist ivlistimpl'

rm -r -f *.o

for i in $nrnmpiSRC ; do
  mpicc -I. -Inrnoc -Inrniv -c nrnmpi/$i.c
done

for i in $nrnocSRC ; do
  gcc -I. -Inrnoc -Inrniv -c nrnoc/$i.c
done

for i in $nrnivSRC ; do
  g++ -I. -Inrnoc -Inrniv -c nrniv/$i.cpp
done

g++ -I. -Inrnoc -Inrniv -c main.cpp

mpicxx *.o
