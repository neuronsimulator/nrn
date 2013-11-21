set(SOURCES
nrnmpi/nrnmpi.c
nrnmpi/mpispike.c
nrnoc/capac.c
nrnoc/eion.c
nrnoc/finitialize.c
nrnoc/fadvance_core.c
nrnoc/solve_core.c
nrnoc/treeset_core.c
nrnoc/nrnoc_aux.c
nrnoc/register_mech.c
nrnoc/multicore.c
nrnoc/nrntimeout.c
nrniv/netpar.cpp
nrniv/netcvode.cpp
nrniv/cxprop.cpp
nrniv/cvodestb.cpp
nrniv/tqueue.cpp
nrniv/htlist.cpp
nrniv/ivlistimpl.cpp
nrniv/ivvect.cpp
nrniv/mk_mech.cpp
nrniv/nrn_setup.cpp
nrniv/output_spikes.cpp
)

set(LIBRARY corebluron)

include_directories(${CMAKE_SOURCE_DIR}/src)
add_library(${LIBRARY} ${SOURCES})
