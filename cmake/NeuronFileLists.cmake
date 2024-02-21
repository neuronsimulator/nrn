# =======================================================================================
# Lists of header files to install. Difference is whether the dir structure is preserved.
#
# * HEADER_FILES_TO_INSTALL: ${src}/src/dir/header.h -> {bld}/include/header.h
# * STRUCTURED_HEADER_FILES_TO_INSTALL: {src}/src/a/b.h -> {bld}/include/a/b.h
# =======================================================================================
set(STRUCTURED_HEADER_FILES_TO_INSTALL
    neuron/cache/mechanism_range.hpp neuron/container/data_handle.hpp
    neuron/container/generic_data_handle.hpp neuron/container/non_owning_soa_identifier.hpp
    neuron/model_data_fwd.hpp)
set(HEADER_FILES_TO_INSTALL
    gnu/mcran4.h
    gnu/nrnisaac.h
    gnu/nrnran123.h
    nrniv/backtrace_utils.h
    nrniv/bbsavestate.h
    nrnmpi/nrnmpidec.h
    nrnoc/cabvars.h
    nrnoc/md1redef.h
    nrnoc/md2redef.h
    nrnoc/membdef.h
    nrnoc/membfunc.h
    nrnoc/multicore.h
    nrnoc/multisplit.h
    nrnoc/neuron.h
    nrnoc/nmodlmutex.h
    nrnoc/nrn_ansi.h
    nrnoc/nrncvode.h
    nrnoc/nrniv_mf.h
    nrnoc/nrnoc_ml.h
    nrnoc/nrnredef.h
    nrnoc/nrnversionmacros.h
    nrnoc/options.h
    nrnoc/section_fwd.hpp
    nrnoc/treeset.h
    oc/hoc.h
    oc/hoc_membf.h
    oc/hocassrt.h
    oc/hocdec.h
    oc/hocgetsym.h
    oc/hoclist.h
    oc/hocparse.h
    oc/mech_api.h
    oc/memory.hpp
    oc/nrnapi.h
    oc/nrnassrt.h
    oc/nrnmpi.h
    oc/nrnrandom.h
    oc/oc_ansi.h
    oc/ocfunc.h
    oc/ocmisc.h
    oc/parse_with_deps.hpp
    oc/wrap_sprintf.h
    scopmath/crout.hpp
    scopmath/crout_thread.hpp
    scopmath/deflate.hpp
    scopmath/dimplic.hpp
    scopmath/errcodes.hpp
    scopmath/euler.hpp
    scopmath/euler_thread.hpp
    scopmath/newton.hpp
    scopmath/newton_struct.h
    scopmath/newton_thread.hpp
    scopmath/row_view.hpp
    scopmath/runge.hpp
    scopmath/scoplib.h
    scopmath/simeq.hpp
    scopmath/sparse.hpp
    scopmath/sparse_thread.hpp
    scopmath/ssimplic.hpp
    scopmath/ssimplic_thread.hpp
    sparse13/spconfig.h
    sparse13/spmatrix.h)

# =============================================================================
# Lists of headers populated using check_include_files
# =============================================================================
set(NRN_HEADERS_INCLUDE_LIST)

# =============================================================================
# Files in oc directory
# =============================================================================
set(OC_FILE_LIST
    audit.cpp
    axis.cpp
    code.cpp
    code2.cpp
    debug.cpp
    fileio.cpp
    ftime.cpp
    functabl.cpp
    getsym.cpp
    hoc.cpp
    hocusr.cpp
    hoc_init.cpp
    hoc_oop.cpp
    list.cpp
    math.cpp
    oc_mcran4.cpp
    memory.cpp
    mswinprt.cpp
    nonlin.cpp
    ocerf.cpp
    plot.cpp
    plt.cpp
    scoprand.cpp
    settext.cpp
    symbol.cpp
    version.cpp
    x.cpp
    xred.cpp)

# =============================================================================
# Files in nrnoc directory
# =============================================================================
set(NRNOC_FILE_LIST
    cabcode.cpp
    capac.cpp
    clamp.cpp
    container.cpp
    eion.cpp
    extcelln.cpp
    fadvance.cpp
    fstim.cpp
    hocprax.cpp
    init.cpp
    ldifus.cpp
    membfunc.cpp
    memblist.cpp
    nrnnemo.cpp
    nrntimeout.cpp
    nrnversion.cpp
    passive0.cpp
    point.cpp
    psection.cpp
    seclist.cpp
    secref.cpp
    solve.cpp
    synapse.cpp
    treeset.cpp
    multicore.cpp)

set(NRNOC_GENERATED_FILE_LIST nrnversion.h)

# =============================================================================
# Files in ivoc directory
# =============================================================================
set(IVOC_FILE_LIST
    apwindow.cpp
    axis.cpp
    bndedval.cpp
    cbwidget.cpp
    checkpnt.cpp
    epsprint.cpp
    fourier.cpp
    gifimage.cpp
    graph.cpp
    graphvec.cpp
    grglyph.cpp
    grmanip.cpp
    hocmark.cpp
    htlist.cpp
    idraw.cpp
    ivoc.cpp
    ivocmain.cpp
    ivocrand.cpp
    ivocvect.cpp
    matrix.cpp
    mlinedit.cpp
    mymath.cpp
    objcmd.cpp
    oc2iv.cpp
    ocbox.cpp
    ocbrowsr.cpp
    ocdeck.cpp
    ocfile.cpp
    ochelp.cpp
    oclist.cpp
    ocmatrix.cpp
    ocnoiv1.cpp
    ocobserv.cpp
    ocpicker.cpp
    ocpointer.cpp
    ocptrvector.cpp
    octimer.cpp
    pwman.cpp
    rect.cpp
    rubband.cpp
    scene.cpp
    scenepic.cpp
    strfun.cpp
    symchoos.cpp
    utility.cpp
    xmenu.cpp
    xyview.cpp)
if(MINGW)
  list(APPEND IVOC_FILE_LIST ivocwin.cpp)
else()
  list(APPEND IVOC_FILE_LIST field.cpp xdep.cpp)
endif()

# =============================================================================
# Files in nrniv directory
# =============================================================================
set(NRNIV_FILE_LIST
    backtrace_utils.cpp
    bbs.cpp
    bbsavestate.cpp
    bbsdirect.cpp
    bbslocal.cpp
    bbslsrv.cpp
    bbslsrv2.cpp
    bbsrcli.cpp
    bbssrv.cpp
    classreg.cpp
    cxprop.cpp
    datapath.cpp
    finithnd.cpp
    glinerec.cpp
    hocmech.cpp
    impedanc.cpp
    kschan.cpp
    kssingle.cpp
    linmod.cpp
    linmod1.cpp
    matrixmap.cpp
    memory_usage.cpp
    multisplit.cpp
    ndatclas.cpp
    netpar.cpp
    nmodlrandom.cpp
    nonlinz.cpp
    nrncore_write.cpp
    nrncore_write/callbacks/nrncore_callbacks.cpp
    nrncore_write/data/cell_group.cpp
    nrncore_write/data/datum_indices.cpp
    nrncore_write/io/nrncore_io.cpp
    nrncore_write/utils/nrncore_utils.cpp
    nrndae.cpp
    nrnmenu.cpp
    nrnpy.cpp
    nrnste.cpp
    nvector_nrnserial_ld.cpp
    nvector_nrnthread.cpp
    nvector_nrnthread_ld.cpp
    ocbbs.cpp
    ocjump.cpp
    partrans.cpp
    ppshape.cpp
    prcellstate.cpp
    pysecname2sec.cpp
    rotate3d.cpp
    savstate.cpp
    secbrows.cpp
    shape.cpp
    shapeplt.cpp
    spaceplt.cpp
    splitcell.cpp
    symdir.cpp
    vrecord.cpp)

# =============================================================================
# Files in nrncvode directory
# =============================================================================
set(NRNCVODE_FILE_LIST
    cvodeobj.cpp
    cvodestb.cpp
    cvtrset.cpp
    netcvode.cpp
    nrndaspk.cpp
    occvode.cpp
    tqueue.cpp)

# =============================================================================
# Files in sundials directory
# =============================================================================
nrn_create_file_list(
  SUNDIALS_CVODES
  "${PROJECT_SOURCE_DIR}/src/sundials/cvodes"
  cvband.c
  cvbandpre.c
  cvbbdpre.c
  cvdense.c
  cvdiag.c
  cvodea.c
  cvodes.c
  cvodesio.c
  cvspgmr.c)

nrn_create_file_list(
  SUNDIALS_IDA
  "${PROJECT_SOURCE_DIR}/src/sundials/ida"
  idaband.c
  idabbdpre.c
  ida.c
  idadense.c
  idaic.c
  idaio.c
  idaspgmr.c)
nrn_create_file_list(
  SUNDIALS_SHARED
  "${PROJECT_SOURCE_DIR}/src/sundials/shared"
  band.c
  dense.c
  iterative.c
  nvector.c
  nvector_serial.c
  smalldense.c
  spgmr.c
  sundialsmath.c)
set(NRN_SUNDIALS_SRC_FILES ${SUNDIALS_CVODES} ${SUNDIALS_IDA} ${SUNDIALS_SHARED})

set(SPARSE_FILES_LIST bksub.cpp getelm.cpp lineq.cpp prmat.cpp subrows.cpp)

# scopmath sources
set(SCOPMATH_FILES_LIST
    abort.cpp
    advance.cpp
    boundary.cpp
    crank.cpp
    scoperf.cpp
    expfit.cpp
    exprand.cpp
    f2cmisc.cpp
    factoria.cpp
    force.cpp
    gauss.cpp
    getmem.cpp
    harmonic.cpp
    hyperbol.cpp
    invert.cpp
    lag.cpp
    legendre.cpp
    normrand.cpp
    perpulse.cpp
    perstep.cpp
    poisrand.cpp
    poisson.cpp
    praxis.cpp
    pulse.cpp
    ramp.cpp
    revhyper.cpp
    revsawto.cpp
    revsigmo.cpp
    romberg.cpp
    sawtooth.cpp
    sigmoid.cpp
    spline.cpp
    squarewa.cpp
    step.cpp
    threshol.cpp
    tridiag.cpp)

set(NRNMPI_FILES_LIST nrnmpi.cpp memory_usage.cpp bbsmpipack.cpp mpispike.cpp)

# nrnpython sources (only if ${NRN_ENABLE_PYTHON_DYNAMIC} is OFF}
set(NRNPYTHON_FILES_LIST
    nrnpython.cpp
    nrnpy_hoc.cpp
    nrnpy_nrn.cpp
    nrnpy_p2h.cpp
    grids.cpp
    rxd.cpp
    rxd_extracellular.cpp
    rxd_intracellular.cpp
    rxd_vol.cpp
    rxd_marching_cubes.cpp
    rxd_llgramarea.cpp)

# built-in mod files
set(MODFILE_BASE_NAMES
    apcount
    exp2syn
    expsyn
    feature
    hh
    intfire1
    intfire2
    intfire4
    netstim
    oclmp
    passive
    pattern
    ppmark
    stim
    svclmp
    syn
    vclmp)

set(MODLUNIT_FILES_LIST
    consist.cpp
    declare.cpp
    init.cpp
    io.cpp
    kinunit.cpp
    list.cpp
    model.cpp
    nrnunit.cpp
    passn.cpp
    symbol.cpp
    units.cpp
    units1.cpp
    version.cpp)

set(NMODL_FILES_LIST
    consist.cpp
    deriv.cpp
    discrete.cpp
    init.cpp
    io.cpp
    kinetic.cpp
    list.cpp
    modl.cpp
    nocpout.cpp
    noccout.cpp
    parsact.cpp
    netrec_discon.cpp
    simultan.cpp
    solve.cpp
    symbol.cpp
    units.cpp
    version.cpp)

set(IVOS_FILES_LIST observe.cpp resource.cpp)

set(MPI_DYNAMIC_INCLUDE nrnmpi_dynam.h nrnmpi_dynam_cinc nrnmpi_dynam_wrappers.inc)

set(NRN_MUSIC_FILES_LIST nrnmusic.cpp)

# =============================================================================
# Top level directories under src
# =============================================================================
set(NRN_OC_SRC_DIR ${PROJECT_SOURCE_DIR}/src/oc)
set(NRN_NRNOC_SRC_DIR ${PROJECT_SOURCE_DIR}/src/nrnoc)
set(NRN_NRNOC_BUILD_DIR ${PROJECT_BINARY_DIR}/src/nrnoc)
set(NRN_IVOC_SRC_DIR ${PROJECT_SOURCE_DIR}/src/ivoc)
set(NRN_NRNCVODE_SRC_DIR ${PROJECT_SOURCE_DIR}/src/nrncvode)
set(NRN_NRNIV_SRC_DIR ${PROJECT_SOURCE_DIR}/src/nrniv)
set(NRN_MODLUNIT_SRC_DIR ${PROJECT_SOURCE_DIR}/src/modlunit)
set(NRN_NMODL_SRC_DIR ${PROJECT_SOURCE_DIR}/src/nmodl)
set(NRN_IVOS_SRC_DIR ${PROJECT_SOURCE_DIR}/src/ivos)
set(NRN_MUSIC_SRC_DIR ${PROJECT_SOURCE_DIR}/src/neuronmusic)

# =============================================================================
# Create source file lists by gathering from various directories
# =============================================================================
nrn_create_file_list(NRN_OC_SRC_FILES ${NRN_OC_SRC_DIR} ${OC_FILE_LIST})
nrn_create_file_list(NRN_NRNOC_SRC_FILES ${NRN_NRNOC_SRC_DIR} ${NRNOC_FILE_LIST})
nrn_create_file_list(NRN_NRNOC_SRC_FILES ${NRN_NRNOC_BUILD_DIR} ${NRNOC_GENERATED_FILE_LIST})
nrn_create_file_list(NRN_IVOC_SRC_FILES ${NRN_IVOC_SRC_DIR} ${IVOC_FILE_LIST})
nrn_create_file_list(NRN_NRNCVODE_SRC_FILES ${NRN_NRNCVODE_SRC_DIR} ${NRNCVODE_FILE_LIST})
nrn_create_file_list(NRN_NRNIV_SRC_FILES ${NRN_NRNIV_SRC_DIR} ${NRNIV_FILE_LIST})
nrn_create_file_list(NRN_PARALLEL_SRC_FILES ${PROJECT_SOURCE_DIR}/src/nrniv
                     nvector_nrnparallel_ld.cpp)
nrn_create_file_list(NRN_PARALLEL_SRC_FILES ${PROJECT_SOURCE_DIR}/src/sundials/shared
                     nvector_parallel.c)
nrn_create_file_list(NRN_SPARSE_SRC_FILES ${PROJECT_SOURCE_DIR}/src/sparse ${SPARSE_FILES_LIST})
nrn_create_file_list(NRN_SCOPMATH_SRC_FILES ${PROJECT_SOURCE_DIR}/src/scopmath
                     ${SCOPMATH_FILES_LIST})
nrn_create_file_list(NRN_NRNMPI_SRC_FILES ${PROJECT_SOURCE_DIR}/src/nrnmpi ${NRNMPI_FILES_LIST})
nrn_create_file_list(NRN_NRNPYTHON_SRC_FILES ${PROJECT_SOURCE_DIR}/src/nrnpython
                     ${NRNPYTHON_FILES_LIST})
nrn_create_file_list(NRN_MODFILE_BASE_NAMES src/nrnoc ${MODFILE_BASE_NAMES})
nrn_create_file_list(NRN_BIN_SRC_FILES ${PROJECT_SOURCE_DIR}/src/ivoc/ nrnmain.cpp)
nrn_create_file_list(NRN_BIN_SRC_FILES ${PROJECT_SOURCE_DIR}/src/oc/ ockludge.cpp modlreg.cpp)
nrn_create_file_list(NRN_MODLUNIT_SRC_FILES ${NRN_MODLUNIT_SRC_DIR} ${MODLUNIT_FILES_LIST})
nrn_create_file_list(NRN_NMODL_SRC_FILES ${NRN_NMODL_SRC_DIR} ${NMODL_FILES_LIST})
nrn_create_file_list(NRNMPI_DYNAMIC_INCLUDE_FILE ${PROJECT_SOURCE_DIR}/src/nrnmpi
                     ${MPI_DYNAMIC_INCLUDE})
nrn_create_file_list(NRN_IVOS_SRC_FILES ${NRN_IVOS_SRC_DIR} ${IVOS_FILES_LIST})
nrn_create_file_list(NRN_MUSIC_SRC_FILES ${NRN_MUSIC_SRC_DIR} ${NRN_MUSIC_FILES_LIST})
list(APPEND NRN_OC_SRC_FILES ${PROJECT_BINARY_DIR}/src/oc/hocusr.h)

# =============================================================================
# Create mswin install lists needed for setup_exe target
# =============================================================================
if(MINGW)
  set(MSWIN_SRC_DIR ${PROJECT_SOURCE_DIR}/src/mswin)
  nrn_create_file_list(MSWIN_FILES ${PROJECT_SOURCE_DIR}/src/parallel test0.hoc test0.py)
  list(APPEND MSWIN_FILES ${MSWIN_SRC_DIR}/notes.txt)
  nrn_create_file_list(MSWIN_BIN_FILES ${MSWIN_SRC_DIR} nrniv.ico nrniv10.ico nmodl2a.ico)
  nrn_create_file_list(
    MSWIN_LIB_FILES
    ${MSWIN_SRC_DIR}/lib
    bshstart.sh
    neuron2.sh
    neuron3.sh
    mknrndl2.sh
    mknrndll.sh
    modlunit.sh)
  list(APPEND MSWIN_LIB_FILES ${PROJECT_BINARY_DIR}/src/mswin/lib/mknrndll.mak)
endif()
