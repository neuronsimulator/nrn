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
  parse.y bksub.c getelm.c lineq.c subrows.c prmat.c
  code2.c debug.c ocerf.c fileio.c ftime.c
  getsym.c hoc.c hocedit.c math.c nonlin.c
  list.c regexp.c audit.c symbol.c
  version.c xred.c parallel.c functabl.c plot.c plt.c axis.c
  settext.c x.c fmenu.c termio.c isoc99.c
  scoprand.c ${RanFiles} cygwinprt.c nrnfilewrap.c
)
#message(NOTICE " OC_SRC ${OC_SRC}")

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

set_source_files_properties(
  ${nrnocsrcdir}/code.c ${nrnocsrcdir}/hoc_init.c ${nrnocsrcdir}/hoc_oop.c  
  PROPERTIES COMPILE_DEFINITIONS CABLE=1)

set(ivocsrcdir ${PROJECT_SOURCE_DIR}/src/ivoc)
mymklist(IVOC_SRC "${ivocsrcdir}"
  ivoc.cpp xmenu.cpp graph.cpp rubband.cpp grmanip.cpp
  apwindow.cpp pwman.cpp scene.cpp xyview.cpp rect.cpp mymath.cpp
  axis.cpp utility.cpp idraw.cpp symchoos.cpp htlist.cpp
  ocpicker.cpp scenepic.cpp oclist.cpp
  ocbrowsr.cpp objcmd.cpp ocbox.cpp ocfile.cpp hocmark.cpp oc2iv.cpp
  epsprint.cpp ivocvect.cpp ivocrand.cpp
  octimer.cpp ocdeck.cpp bndedval.cpp ochelp.cpp checkpnt.cpp
  graphvec.cpp strfun.cpp ocobserv.cpp fourier.cpp
  cbwidget.cpp matrix.cpp ocmatrix.cpp
  ocpointer.cpp gifimage.cpp ocnoiv1.cpp grglyph.cpp mlinedit.cpp
  ivocman1.cpp ocptrvector.cpp
)

#$(sysdep_sources)

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

#$(PARSRC1)

set_property(
  SOURCE ${nrnocsrcdir}/code.c ${nrnocsrcdir}/hoc_init.c ${nrnocsrcdir}/hoc_oop.c
  APPEND
  PROPERTY COMPILE_DEFINITIONS
    CABLE=1
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
    USEMATRIX=1 USECVODE=1 CABLE=1
  )

set (base ${OC_SRC} ${NRNOC_SRC} ${IVOC_SRC} ${NRNIV_SRC})
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
list(APPEND base_include ${PROJECT_SOURCE_DIR}/src/sundials/cvodes)
list(APPEND base_include ${PROJECT_SOURCE_DIR}/src/sundials/ida)
list(APPEND base_include ${PROJECT_SOURCE_DIR}/../ivcmake/build/install/include)

configure_ac_file(nrnmpiuse.h src/oc)
configure_ac_file(nrnrtuse.h src/oc)
configure_ac_file(nrnpthread.h src/oc)
configure_ac_file(nrnconfigargs.h src/nrnoc)
configure_ac_file(nrnpython_config.h src/nrnpython)
configure_ac_file(bbsconf.h src/parallel)
configure_ac_file(nrnneosm.h src/nrncvode)
configure_ac_file(sundials_config.h src/sundials)
configure_ac_file(mos2nrn.h src/uxnrnbbs)
configure_ac_file(ivstream.h src/ivos)
configure_ac_file(njconf.h src/nrnjava)
configure_ac_file(nmodlconf.h .)

# bison parser generation
find_package(BISON)

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

add_library(nrniv SHARED ${base})
