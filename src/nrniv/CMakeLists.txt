# The old build (autoconf) built executables in oc, nrnoc, ivoc, nrniv
# as well as libraries there and in a dozen or so other places.
# For cmake I'm experimenting with a single libnrniv.so that combines
# many of files in those folders, both for simplicity, and to avoid
# issues of dependency among the old libraries. Also the oc, nrnoc, and
# ivoc executables are pretty well useless these days.
# Other groups can, and should be, factored out

# files that compile and go into libnrniv.so

set(ocsrcdir ${PROJECT_SOURCE_DIR}/src/oc)
set(RanFiles isaac64.c mcran4.c nrnisaac.c nrnran123.c)
# append to list (first arg) by prepending second arg/ to all remaining args
mymklist(OC_SRC "${ocsrcdir}"
  parse.c bksub.c getelm.c lineq.c subrows.c prmat.c
  code2.c debug.c ocerf.c fileio.c ftime.c
  getsym.c hoc.c hocedit.c math.c nonlin.c
  list.c regexp.c audit.c symbol.c
  version.c xred.c parallel.c functabl.c plot.c plt.c axis.c
  settext.c x.c fmenu.c isoc99.c
  scoprand.c ${RanFiles} cygwinprt.c nrnfilewrap.c
  #get termio.c from src/emacs
)
#message(NOTICE " OC_SRC ${OC_SRC}")
set_property(
  SOURCE ${OC_SRC}
  APPEND
  PROPERTY COMPILE_DEFINITIONS
    OOP=1 HOC=1 INTERVIEWS=1
)
#$(nrnmpidynam) nothing or nrnmpi_dynam.c

set(nrnocsrcdir ${PROJECT_SOURCE_DIR}/src/nrnoc)
mymklist(NRNOC_SRC ${nrnocsrcdir}
  cabcode.c capac.c clamp.c eion.c
  extcelln.c fadvance.c fstim.c init.c nrnnemo.c point.c
  psection.c solve.c synapse.c treeset.c hoc_init.c code.c hoc_oop.c
  seclist.c method3.c hocprax.c secref.c ldifus.c hocusr.c nrnversion.c
  nrnversion.h nrntimeout.c passive0.c
)
list(APPEND NRNOC_SRC ${PROJECT_BINARY_DIR}/src/nrnoc/hocusr.h)

set_property(
  SOURCE ${nrnocsrcdir}/code.c ${nrnocsrcdir}/hoc_init.c ${nrnocsrcdir}/hoc_oop.c  
  APPEND
  PROPERTY COMPILE_DEFINITIONS
    CABLE=1
)

set(ivocsrcdir ${PROJECT_SOURCE_DIR}/src/ivoc)
mymklist(IVOC_SRC "${ivocsrcdir}"
  ivocmain.cpp ivoc.cpp xmenu.cpp graph.cpp rubband.cpp grmanip.cpp
  apwindow.cpp pwman.cpp scene.cpp xyview.cpp rect.cpp mymath.cpp
  axis.cpp utility.cpp idraw.cpp symchoos.cpp htlist.cpp
  ocpicker.cpp scenepic.cpp oclist.cpp
  ocbrowsr.cpp objcmd.cpp ocbox.cpp ocfile.cpp hocmark.cpp oc2iv.cpp
  epsprint.cpp ivocvect.cpp ivocrand.cpp
  octimer.cpp ocdeck.cpp bndedval.cpp ochelp.cpp checkpnt.cpp
  graphvec.cpp strfun.cpp ocobserv.cpp fourier.cpp
  cbwidget.cpp matrix.cpp ocmatrix.cpp
  ocpointer.cpp gifimage.cpp ocnoiv1.cpp grglyph.cpp mlinedit.cpp
  ocptrvector.cpp
)

#$(sysdep_sources)
mymklist(IVOC_SRC "${ivocsrcdir}"
  xdep.cpp field.cpp
)

# nrniv sources
set(nrnivsrcdir ${PROJECT_SOURCE_DIR}/src/nrniv)
mymklist(NRNIV_SRC "${nrnivsrcdir}"
  nrnmenu.cpp shape.cpp classreg.cpp rotate3d.cpp
  datapath.cpp symdir.cpp spaceplt.cpp shapeplt.cpp ppshape.cpp
  secbrows.cpp ndatclas.cpp impedanc.cpp savstate.cpp hocmech.cpp
  ocjump.cpp vrecord.cpp cvodestb.cpp occvode.cpp
  cvodeobj.cpp cvtrset.cpp tqueue.cpp netcvode.cpp singlech.cpp
  nrndaspk.cpp glinerec.cpp cxprop.cpp
  ocbbs.cpp bbs.cpp bbslocal.cpp bbslsrv.cpp
  bbsrcli.cpp bbsdirect.cpp bbslsrv2.cpp bbssrv.cpp
  linmod.cpp linmod1.cpp cachevec.cpp
  kschan.cpp kssingle.cpp nonlinz.cpp finithnd.cpp nrnste.cpp
  nrnrtime.cpp nvector_nrnthread.c nrnpy.cpp prcellstate.cpp
  nvector_nrnthread_ld.c nvector_nrnserial_ld.c
  bgpmeminfo.c
  netpar.cpp partrans.cpp splitcell.cpp multisplit.cpp
  bbsavestate.cpp nrnbbcore_write.cpp pysecname2sec.cpp
  nrndae.cpp matrixmap.cpp geometry3d.cpp
)
# goes into libnrniv.so unless ENABLE_MPI is OFF
mymklist(PARSRC1 "${PROJECT_SOURCE_DIR}/src/nrniv" nvector_nrnparallel_ld.c)

set_property(
  SOURCE ${nrnocsrcdir}/code.c ${nrnocsrcdir}/hoc_init.c ${nrnocsrcdir}/hoc_oop.c
  APPEND
  PROPERTY COMPILE_DEFINITIONS
    CABLE=1
  )

set_property(
  SOURCE ${PROJECT_SOURCE_DIR}/src/nrniv/nrnpy.cpp
    ${PROJECT_SOURCE_DIR}/src/nrnmpi/nrnmpi_dynam.c
  APPEND
  PROPERTY COMPILE_DEFINITIONS NRNCMAKE
)

set_property(
  SOURCE ${IVOC_SRC}
  APPEND
  PROPERTY COMPILE_DEFINITIONS
    USEMATRIX=1 USEGNU=1 USEBBS=1
  )

set_property(
  SOURCE ${NRNIV_SRC}
  APPEND
  PROPERTY COMPILE_DEFINITIONS
    USEMATRIX=1 USECVODE=1 CABLE=1 USEBBS=1 OOP=1
  )

# sundials sources
mymklist(SUNDIALS_SRC "${PROJECT_SOURCE_DIR}/src/sundials/cvodes"
        cvband.c
        cvbandpre.c
        cvbbdpre.c
        cvdense.c
        cvdiag.c
        cvodea.c
        cvodes.c
        cvodesio.c
        cvspgmr.c
)
mymklist(SUNDIALS_SRC "${PROJECT_SOURCE_DIR}/src/sundials/ida"
        idaband.c
        idabbdpre.c
        ida.c
        idadense.c
        idaic.c
        idaio.c
        idaspgmr.c
)
mymklist(SUNDIALS_SRC "${PROJECT_SOURCE_DIR}/src/sundials/shared"
  band.c dense.c iterative.c
  nvector.c nvector_serial.c smalldense.c spgmr.c sundialsmath.c
)
# goes into libnrniv.so unless ENABLE_MPI is OFF
mymklist(PARSRC1 "${PROJECT_SOURCE_DIR}/src/sundials/shared" nvector_parallel.c)

# meschach matrix sources
mymklist(MESCH_SRC "${PROJECT_SOURCE_DIR}/src/mesch"
arnoldi.c bdfactor.c bkpfacto.c chfactor.c
  arnoldi.c bdfactor.c bkpfacto.c chfactor.c
        conjgrad.c copy.c dmacheps.c err.c extras.c fft.c givens.c hessen.c
        hsehldr.c init.c iter0.c iternsym.c itersym.c ivecop.c lanczos.c
        lufactor.c machine.c matlab.c matop.c matrixio.c meminfo.c memory.c
        memstat.c mfunc.c norm.c otherio.c pxop.c qrfactor.c schur.c
        solve.c sparse.c sparseio.c spbkp.c spchfctr.c splufctr.c sprow.c
        spswap.c submat.c svd.c symmeig.c update.c vecop.c version.c
        zcopy.c zfunc.c zgivens.c zhessen.c zhsehldr.c zlufctr.c zmachine.c
        zmatio.c zmatlab.c zmatop.c zmemory.c znorm.c zqrfctr.c zschur.c
        zsolve.c zvecop.c
)

# sparse13 matrix sources
mymklist(SPARSE13_SRC "${PROJECT_SOURCE_DIR}/src/sparse13"
  spalloc.c spbuild.c spfactor.c spoutput.c
        spsolve.c sputils.c
        cspalloc.c cspbuild.c cspfactor.c cspoutput.c
        cspsolve.c csputils.c
)

if (0) # seem to be listed in OC_SRC
# sparse matrix sources
mymklist(SPARSE_SRC "${PROJECT_SOURCE_DIR}/src/sparse"
  bksub.c getelm.c lineq.c lineq.h prmat.c subrows.c
)
endif()

# scopmath sources
mymklist(SCOPMATH_SRC "${PROJECT_SOURCE_DIR}/src/scopmath"
  abort.c adams.c adeuler.c adrunge.c advance.c
        boundary.c crank.c crout.c csoda.c csodabnc.c deflate.c
        dimplic.c scoperf.c euler.c expfit.c exprand.c f2cmisc.c factoria.c
        force.c gauss.c gear.c getmem.c harmonic.c heun.c hyperbol.c
        invert.c lag.c legendre.c newton.c normrand.c perpulse.c
        perstep.c poisrand.c poisson.c praxis.c pulse.c quad.c ramp.c
        revhyper.c revsawto.c revsigmo.c romberg.c runge.c
        sawtooth.c schedule.c seidel.c sigmoid.c simeq.c simplex.c
        sparse.c vsparse.c spline.c squarewa.c ssimplic.c step.c
        threshol.c tridiag.c sparse_thread.c newton_thread.c crout_thread.c
        ssimplic_thread.c
)
set_property(
  SOURCE ${SCOPMATH_SRC}
  APPEND
  PROPERTY COMPILE_DEFINITIONS HOC=1
)
set_property(
  SOURCE ${SCOPMATH_SRC}
  APPEND
  #not available with cmake 3.10.2
  #PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/src/scopmath"
  PROPERTY COMPILE_FLAGS "-I${PROJECT_SOURCE_DIR}/src/scopmath"
)


# memacs sources
mymklist(MEMACS_SRC "${PROJECT_SOURCE_DIR}/src/memacs"
  ansi.c basic.c bind.c buffer.c display.c file.c
        fileio.c line.c lock.c main1.c random.c region.c
        search.c spawn.c tcap.c termio.c window.c word.c
)

# readline sources
mymklist(READLINE_SRC "${PROJECT_SOURCE_DIR}/src/readline"
#  readline.c history.c funmap.c keymaps.c
)

# nrnmpi sources (only if "${NRNMPI_DYNAMIC}" matches "NO")
mymklist(NRNMPI_SRC "${PROJECT_SOURCE_DIR}/src/nrnmpi"
  nrnmpi.c bbsmpipack.c mpispike.c nrnrt.c
)

set (NRNMPI_SRC ${NRNMPI_SRC} PARENT_SCOPE)

# ancient gnu double_avec and random distributions and generators
mymklist(NRNGNU_SRC "${PROJECT_SOURCE_DIR}/src/gnu"
  ACG.cpp Binomial.cpp Complex.cpp DiscUnif.cpp
  Erlang.cpp Geom.cpp HypGeom.cpp LogNorm.cpp MLCG.cpp NegExp.cpp
  Normal.cpp Poisson.cpp RNG.cpp Random.cpp RndInt.cpp SmplHist.cpp
  SmplStat.cpp Uniform.cpp Weibull.cpp builtin.cpp d_avec.cpp d_vec.cpp
)

# nrnpython sources (only if "${NRNPYTHON_DYNAMIC}" MATCHES "NO")
mymklist(NRNPYTHON_SRC "${PROJECT_SOURCE_DIR}/src/nrnpython"
  nrnpython.cpp nrnpy_hoc.cpp nrnpy_nrn.cpp nrnpy_p2h.cpp
  grids.cpp rxd.cpp rxd_extracellular.cpp rxd_intracellular.cpp
  rxd_vol.cpp rxd_marching_cubes.c rxd_llgramarea.c
)
set (NRNPYTHON_SRC ${NRNPYTHON_SRC} PARENT_SCOPE)

# built-in mod files
mymklist(modfile_bases "${PROJECT_SOURCE_DIR}/src/nrnoc"
  apcount feature intfire2 oclmp ppmark syn
  exp2syn hh intfire4 passive stim vclmp
  expsyn intfire1 netstim pattern svclmp
)
myprepostlist(MODFILE_MOD "" ".mod"
  ${modfile_bases}
)
myprepostlist(MODFILE_C "" ".c"
  ${modfile_bases}
)

set_source_files_properties(${MODFILE_C}
  PROPERTIES GENERATED TRUE
)

#nmodl sources
set(nmodlsrcdir ${PROJECT_SOURCE_DIR}/src/nmodl)
mymklist(NMODL_SRC "${nmodlsrcdir}"
  parse1.c diffeq.c lex.c consist.c deriv.c discrete.c
  init.c io.c kinetic.c list.c modl.c nocpout.c noccout.c parsact.c
  netrec_discon.c
  partial.c sens.c simultan.c solve.c symbol.c units.c version.c
)

set_property(
  SOURCE ${NMODL_SRC}
  APPEND
  PROPERTY COMPILE_DEFINITIONS
    NMODL=1 NOCMODL=1 CVODE=1 VECTORIZE=1
  )



BISON_TARGET(nocmodlparser ${nmodlsrcdir}/parse1.y ${nmodlsrcdir}/parse1.c
  DEFINES_FILE ${nmodlsrcdir}/parse1.h)

BISON_TARGET(nocmodlparser ${nmodlsrcdir}/diffeq.y ${nmodlsrcdir}/diffeq.c
  DEFINES_FILE ${nmodlsrcdir}/diffeq.h)

FLEX_TARGET(nocmodllexer ${nmodlsrcdir}/lex.l ${nmodlsrcdir}/lex.c)

add_executable(nocmodl ${NMODL_SRC})


macro(nocmodl_mod_to_c bname)
add_custom_command(
  OUTPUT ${bname}.c
  COMMAND
    nocmodl ${bname}.mod
  COMMAND sed "'s/_reg()/_reg_()/'" ${bname}.c > ${bname}.c.tmp
  COMMAND mv ${bname}.c.tmp ${bname}.c
  DEPENDS ${PROJECT_BINARY_DIR}/bin/nocmodl ${bname}.mod
)
endmacro()

foreach(name ${modfile_bases})
  nocmodl_mod_to_c(${name})
endforeach(name)

set (base ${OC_SRC} ${NRNOC_SRC} ${IVOC_SRC} ${NRNIV_SRC})
set (nrnivlibsrc ${base} ${SUNDIALS_SRC} ${MESCH_SRC}
  ${SPARSE13_SRC} ${SCOPMATH_SRC} ${MEMACS_SRC} ${READLINE_SRC}
  ${NRNGNU_SRC} ${MODFILE_C}
)

if (ENABLE_MPI)
  list(APPEND nrnivlibsrc ${PARSRC1})
endif()

if("${NRNMPI_DYNAMIC}" MATCHES "NO")
  list(APPEND nrnivlibsrc ${NRNMPI_SRC})
else()
  list(APPEND nrnivlibsrc ${PROJECT_SOURCE_DIR}/src/nrnmpi/nrnmpi_dynam.c)

  mymklist(NRNMPI_DYNAM_INC "${PROJECT_SOURCE_DIR}/src/nrnmpi"
    nrnmpi_dynam.h nrnmpi_dynam_cinc nrnmpi_dynam_wrappers.inc)

  add_custom_command(OUTPUT ${NRNMPI_DYNAM_INC}
    COMMAND sh mkdynam.sh
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/nrnmpi
    DEPENDS ${PROJECT_SOURCE_DIR}/src/nrnmpi/mkdynam.sh
      ${PROJECT_SOURCE_DIR}/src/nrnmpi/nrnmpidec.h
  )

  set_source_files_properties(${PROJECT_SOURCE_DIR}/src/nrnmpi/nrnmpi_dynam.c
    PROPERTIES OBJECT_DEPENDS ${NRNMPI_DYNAM_INC})

endif()

if ("${NRNPYTHON_DYNAMIC}" MATCHES "NO")
  list(APPEND nrnivlibsrc ${NRNPYTHON_SRC})
endif()

# since everone #include <../../nrnconf.h>
set (nrnconfhelp ${PROJECT_BINARY_DIR}/src/nrnoc ${PROJECT_BINARY_DIR}/src/oc)
set (base_include ${nrnconfhelp} ${nrnivsrcdir} ${ivocsrcdir} ${nrnocsrcdir} ${ocsrcdir})
list(APPEND base_include ${PROJECT_SOURCE_DIR}/src/memacs)
list(APPEND base_include ${PROJECT_SOURCE_DIR}/src/sparse)
list(APPEND base_include ${PROJECT_SOURCE_DIR}/src/sparse13)
list(APPEND base_include ${PROJECT_BINARY_DIR})
list(APPEND base_include ${PROJECT_SOURCE_DIR}/src)
list(APPEND base_include ${PROJECT_SOURCE_DIR}/src/nrnpython)
list(APPEND base_include ${PROJECT_BINARY_DIR}/src/nrnpython)
list(APPEND base_include ${PROJECT_SOURCE_DIR}/src/nrnmpi)
list(APPEND base_include ${PROJECT_SOURCE_DIR}/src/gnu)
list(APPEND base_include ${PROJECT_SOURCE_DIR}/src/mesch)
list(APPEND base_include ${PROJECT_SOURCE_DIR}/src/parallel)
list(APPEND base_include ${PROJECT_BINARY_DIR}/src/parallel)
list(APPEND base_include ${PROJECT_SOURCE_DIR}/src/nrncvode)
list(APPEND base_include ${PROJECT_BINARY_DIR}/src/nrncvode)
list(APPEND base_include ${PROJECT_BINARY_DIR}/src/sundials)
list(APPEND base_include ${PROJECT_SOURCE_DIR}/src/sundials)
list(APPEND base_include ${PROJECT_SOURCE_DIR}/src/sundials/shared)
list(APPEND base_include ${PROJECT_BINARY_DIR}/src/sundials/shared)
list(APPEND base_include ${PROJECT_SOURCE_DIR}/src/sundials/cvodes)
list(APPEND base_include ${PROJECT_SOURCE_DIR}/src/sundials/ida)

#avoid sundials/shared/nvector_serial.c error for #include <../../../nrnconf.h>
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/src/sundials/shared)

BISON_TARGET(ocparser ${ocsrcdir}/parse.y ${ocsrcdir}/parse.c
  DEFINES_FILE ${ocsrcdir}/parse.h)

set_source_files_properties(${nrnocsrcdir}/code.c
  PROPERTIES OBJECT_DEPENDS ${ocsrcdir}/parse.h)

add_custom_command(OUTPUT ${nrnocsrcdir}/nrnversion.h
  COMMAND ${PROJECT_SOURCE_DIR}/git2nrnversion_h.sh ${PROJECT_SOURCE_DIR} > ${nrnocsrcdir}/nrnversion.h)

set_source_files_properties(${nrnocsrcdir}/nrnversion.c
  PROPERTIES OBJECT_DEPENDS ${PROJECT_SOURCE_DIR}/src/nrnoc/nrnversion.h)

  message(NOTICE " ${PROJECT_BINARY_DIR}/src/nrnoc/hocusr.h")
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src/nrnoc/hocusr.h
  COMMAND ${CMAKE_C_COMPILER} -E -I${nrnocsrcdir} -I${ocsrcdir} ${nrnocsrcdir}/neuron.h > neuron.tmp1
  COMMAND sed "/^#/d" neuron.tmp1 > neuron.tmp2
  COMMAND python ${ocsrcdir}/mk_hocusr_h.py < neuron.tmp2 > temp2hoc
  COMMAND sed "s/\\\"nrnhoc_topology\\\"/\\\"topology\\\"/" temp2hoc > ${PROJECT_BINARY_DIR}/src/nrnoc/hocusr.h
  DEPENDS ${nrnocsrcdir}/neuron.h ${ocsrcdir}/mk_hocusr_h.py
  )

set_source_files_properties(${ocsrcdir}/hocusr.c
  PROPERTIES OBJECT_DEPENDS ${PROJECT_BINARY_DIR}/src/nrnoc/hocusr.h)

#message(STATUS " base is ${base}")

include_directories(${base_include})
if (ENABLE_PYTHON)
  include_directories(${PYTHON_INCLUDE_DIRS})
endif()

add_library(nrniv_lib SHARED ${nrnivlibsrc})
target_link_libraries(nrniv_lib ${X11_LIBRARIES} readline)

if (ENABLE_PYTHON AND "${NRNPYTHON_DYNAMIC}" MATCHES "NO")
target_link_libraries(nrniv_lib ${PYTHON_LIBRARIES})
endif()

if (ENABLE_MPI AND "${NRNMPI_DYNAMIC}" MATCHES "NO")
target_link_libraries(nrniv_lib ${MPI_C_LIBRARIES})
endif()

if (ENABLE_MPI AND "${NRNMPI_DYNAMIC}" MATCHES "YES")
  add_library(nrnmpi_lib SHARED ${NRNMPI_SRC})
  target_link_libraries(nrnmpi_lib ${MPI_C_LIBRARIES})
  set_property(TARGET nrnmpi_lib PROPERTY OUTPUT_NAME nrnmpi)
  install(TARGETS nrnmpi_lib DESTINATION lib)
endif()

if (ENABLE_INTERVIEWS)
  # handy trick to see that the correct iv headers are visible:
  get_target_property(iv_interface_includes iv_interviews INTERFACE_INCLUDE_DIRECTORIES)
  message("IV interface includes: ${iv_interface_includes}")
  include_directories(${iv_interface_includes})
  target_link_libraries(nrniv_lib iv_interviews)
endif()

set_property(TARGET nrniv_lib PROPERTY OUTPUT_NAME nrniv)

mymklist(nrnbinsrc "${PROJECT_SOURCE_DIR}/src/ivoc/"
  nrnmain.cpp
)

mymklist(nrnbinsrc "${PROJECT_SOURCE_DIR}/src/oc/"
  ockludge.c modlreg.c
)

add_executable(nrniv ${nrnbinsrc})
target_link_libraries(nrniv nrniv_lib ${INTERNAL_READLINE} ${Readline_LIBRARY})

install(TARGETS nrniv nocmodl DESTINATION bin)
install(TARGETS nrniv_lib DESTINATION lib)

