# The old build (autoconf) built executables in oc, nrnoc, ivoc, nrniv
# as well as libraries there and in a dozen or so other places.
# For cmake I'm experimenting with a single libnrniv.so that combines
# many of files in those folders, both for simplicity, and to avoid
# issues of dependency among the old libraries. Also the oc, nrnoc, and
# ivoc executables are pretty well useless these days.
# Other groups can, and should be, factored out

# files that compile and go into libnrniv.so

set(ocsrcdir ${PROJECT_SOURCE_DIR}/src/oc)
set(OC_SRC
  ${ocsrcdir}/parse.y
  ${ocsrcdir}/bksub.c
  ${ocsrcdir}/getelm.c
  ${ocsrcdir}/lineq.c
  ${ocsrcdir}/subrows.c
  ${ocsrcdir}/prmat.c
  ${ocsrcdir}/code2.c
  ${ocsrcdir}/debug.c
  ${ocsrcdir}/ocerf.c
  ${ocsrcdir}/fileio.c
  ${ocsrcdir}/ftime.c
  ${ocsrcdir}/getsym.c
  ${ocsrcdir}/hoc.c
  ${ocsrcdir}/hocedit.c
  ${ocsrcdir}/math.c
  ${ocsrcdir}/nonlin.c
  ${ocsrcdir}/list.c
  ${ocsrcdir}/regexp.c
  ${ocsrcdir}/audit.c
  ${ocsrcdir}/symbol.c
  ${ocsrcdir}/version.c
  ${ocsrcdir}/xred.c
  ${ocsrcdir}/parallel.c
  ${ocsrcdir}/functabl.c
  ${ocsrcdir}/plot.c
  ${ocsrcdir}/plt.c
  ${ocsrcdir}/axis.c
  ${ocsrcdir}/settext.c
  ${ocsrcdir}/x.c
  ${ocsrcdir}/fmenu.c
  ${ocsrcdir}/termio.c
  ${ocsrcdir}/isoc99.c
  ${ocsrcdir}/scoprand.c
  ${ocsrcdir}/cygwinprt.c
  ${ocsrcdir}/nrnfilewrap.c
  ${ocsrcdir}/isaac64.c
  ${ocsrcdir}/mcran4.c
  ${ocsrcdir}/nrnisaac.c
  ${ocsrcdir}/nrnran123.c)

#$(nrnmpidynam) nothing or nrnmpi_dynam.c

#message(STATUS " OC_SRC is ${OC_SRC}")

set(nrnocsrcdir ${PROJECT_SOURCE_DIR}/src/nrnoc)
set(NRNOC_SRC
  ${nrnocsrcdir}/cabcode.c
  ${nrnocsrcdir}/capac.c
  ${nrnocsrcdir}/clamp.c
  ${nrnocsrcdir}/eion.c
  ${nrnocsrcdir}/extcelln.c
  ${nrnocsrcdir}/fadvance.c
  ${nrnocsrcdir}/fstim.c
  ${nrnocsrcdir}/init.c
  ${nrnocsrcdir}/nrnnemo.c
  ${nrnocsrcdir}/point.c
  ${nrnocsrcdir}/psection.c
  ${nrnocsrcdir}/solve.c
  ${nrnocsrcdir}/synapse.c
  ${nrnocsrcdir}/treeset.c
  ${nrnocsrcdir}/hoc_init.c
  ${nrnocsrcdir}/code.c
  ${nrnocsrcdir}/hoc_oop.c
  ${nrnocsrcdir}/seclist.c
  ${nrnocsrcdir}/method3.c
  ${nrnocsrcdir}/hocprax.c
  ${nrnocsrcdir}/secref.c
  ${nrnocsrcdir}/ldifus.c
  ${nrnocsrcdir}/hocusr.c
  ${PROJECT_BINARY_DIR}/src/nrnoc/hocusr.h
  ${nrnocsrcdir}/nrnversion.c
  ${nrnocsrcdir}/nrnversion.h
  ${nrnocsrcdir}/nrntimeout.c
  ${nrnocsrcdir}/passive0.c)

set_source_files_properties(
  ${nrnocsrcdir}/code.c ${nrnocsrcdir}/hoc_init.c ${nrnocsrcdir}/hoc_oop.c  
  PROPERTIES COMPILE_DEFINITIONS CABLE=1)

set(ivocsrcdir ${PROJECT_SOURCE_DIR}/src/ivoc)
set(IVOC_SRC
  ${ivocsrcdir}/ivoc.cpp
  ${ivocsrcdir}/xmenu.cpp
  ${ivocsrcdir}/graph.cpp
  ${ivocsrcdir}/rubband.cpp
  ${ivocsrcdir}/grmanip.cpp
  ${ivocsrcdir}/apwindow.cpp
  ${ivocsrcdir}/pwman.cpp
  ${ivocsrcdir}/scene.cpp
  ${ivocsrcdir}/xyview.cpp
  ${ivocsrcdir}/rect.cpp
  ${ivocsrcdir}/mymath.cpp
  ${ivocsrcdir}/axis.cpp
  ${ivocsrcdir}/utility.cpp
  ${ivocsrcdir}/idraw.cpp
  ${ivocsrcdir}/symchoos.cpp
  ${ivocsrcdir}/htlist.cpp
  ${ivocsrcdir}/ocpicker.cpp
  ${ivocsrcdir}/scenepic.cpp
  ${ivocsrcdir}/oclist.cpp
  ${ivocsrcdir}/ocbrowsr.cpp
  ${ivocsrcdir}/objcmd.cpp
  ${ivocsrcdir}/ocbox.cpp
  ${ivocsrcdir}/ocfile.cpp
  ${ivocsrcdir}/hocmark.cpp
  ${ivocsrcdir}/oc2iv.cpp
  ${ivocsrcdir}/epsprint.cpp
  ${ivocsrcdir}/ivocvect.cpp
  ${ivocsrcdir}/ivocrand.cpp
  ${ivocsrcdir}/octimer.cpp
  ${ivocsrcdir}/ocdeck.cpp
  ${ivocsrcdir}/bndedval.cpp
  ${ivocsrcdir}/ochelp.cpp
  ${ivocsrcdir}/checkpnt.cpp
  ${ivocsrcdir}/graphvec.cpp
  ${ivocsrcdir}/strfun.cpp
  ${ivocsrcdir}/ocobserv.cpp
  ${ivocsrcdir}/fourier.cpp
  ${ivocsrcdir}/cbwidget.cpp
  ${ivocsrcdir}/matrix.cpp
  ${ivocsrcdir}/ocmatrix.cpp
  ${ivocsrcdir}/ocpointer.cpp
  ${ivocsrcdir}/gifimage.cpp
  ${ivocsrcdir}/ocnoiv1.cpp
  ${ivocsrcdir}/grglyph.cpp
  ${ivocsrcdir}/mlinedit.cpp
  ${ivocsrcdir}/ivocman1.cpp
  ${ivocsrcdir}/ocptrvector.cpp)

#$(sysdep_sources)

set(nrnivsrcdir ${PROJECT_SOURCE_DIR}/src/nrniv)
set(NRNIV_SRC
  ${nrnivsrcdir}/nrnmenu.cpp
  ${nrnivsrcdir}/shape.cpp
  ${nrnivsrcdir}/classreg.cpp
  ${nrnivsrcdir}/rotate3d.cpp
  ${nrnivsrcdir}/datapath.cpp
  ${nrnivsrcdir}/symdir.cpp
  ${nrnivsrcdir}/spaceplt.cpp
  ${nrnivsrcdir}/shapeplt.cpp
  ${nrnivsrcdir}/ppshape.cpp
  ${nrnivsrcdir}/secbrows.cpp
  ${nrnivsrcdir}/ndatclas.cpp
  ${nrnivsrcdir}/impedanc.cpp
  ${nrnivsrcdir}/savstate.cpp
  ${nrnivsrcdir}/hocmech.cpp
  ${nrnivsrcdir}/ocjump.cpp
  ${nrnivsrcdir}/vrecord.cpp
  ${nrnivsrcdir}/cvodestb.cpp
  ${nrnivsrcdir}/occvode.cpp
  ${nrnivsrcdir}/cvodeobj.cpp
  ${nrnivsrcdir}/cvtrset.cpp
  ${nrnivsrcdir}/tqueue.cpp
  ${nrnivsrcdir}/netcvode.cpp
  ${nrnivsrcdir}/singlech.cpp
  ${nrnivsrcdir}/nrndaspk.cpp
  ${nrnivsrcdir}/glinerec.cpp
  ${nrnivsrcdir}/cxprop.cpp
  ${nrnivsrcdir}/ocbbs.cpp
  ${nrnivsrcdir}/bbs.cpp
  ${nrnivsrcdir}/bbslocal.cpp
  ${nrnivsrcdir}/bbslsrv.cpp
  ${nrnivsrcdir}/bbsrcli.cpp
  ${nrnivsrcdir}/bbsdirect.cpp
  ${nrnivsrcdir}/bbslsrv2.cpp
  ${nrnivsrcdir}/bbssrv.cpp
  ${nrnivsrcdir}/linmod.cpp
  ${nrnivsrcdir}/linmod1.cpp
  ${nrnivsrcdir}/cachevec.cpp
  ${nrnivsrcdir}/kschan.cpp
  ${nrnivsrcdir}/kssingle.cpp
  ${nrnivsrcdir}/nonlinz.cpp
  ${nrnivsrcdir}/finithnd.cpp
  ${nrnivsrcdir}/nrnste.cpp
  ${nrnivsrcdir}/nrnrtime.cpp
  ${nrnivsrcdir}/nvector_nrnthread.c
  ${nrnivsrcdir}/nrnpy.cpp
  ${nrnivsrcdir}/prcellstate.cpp
  ${nrnivsrcdir}/nvector_nrnthread_ld.c
  ${nrnivsrcdir}/nvector_nrnserial_ld.c
  ${nrnivsrcdir}/bgpmeminfo.c
  ${nrnivsrcdir}/netpar.cpp
  ${nrnivsrcdir}/partrans.cpp
  ${nrnivsrcdir}/splitcell.cpp
  ${nrnivsrcdir}/multisplit.cpp
  ${nrnivsrcdir}/bbsavestate.cpp
  ${nrnivsrcdir}/nrnbbcore_write.cpp
  ${nrnivsrcdir}/pysecname2sec.cpp
  ${nrnivsrcdir}/nrndae.cpp
  ${nrnivsrcdir}/matrixmap.cpp
  ${nrnivsrcdir}/geometry3d.cpp
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
