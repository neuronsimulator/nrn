# =============================================================================
# Lists of header files to install
# =============================================================================
set(HEADER_FILES_TO_INSTALL
    bbsavestate.h
    cabvars.h
    cspmatrix.h
    cspredef.h
    hoc.h
    hoc_membf.h
    hocassrt.h
    hocdec.h
    hocgetsym.h
    hoclist.h
    hocparse.h
    isoc99.h
    ivstream.h
    mcran4.h
    md1redef.h
    md2redef.h
    mech_api.h
    membdef.h
    membfunc.h
    multicore.h
    multisplit.h
    neuron.h
    newton_struct.h
    nmodlmutex.h
    nrn_ansi.h
    nrnapi.h
    nrnassrt.h
    nrncvode.h
    nrnisaac.h
    nrniv_mf.h
    nrnoc_ml.h
    nrnmpi.h
    nrnmpidec.h
    nrnrandom.h
    nrnran123.h
    nrnredef.h
    nrnversionmacros.h
    oc_ansi.h
    ocfunc.h
    ocmisc.h
    options.h
    parse_with_deps.hpp
    scoplib.h
    section.h
    spconfig.h
    spmatrix.h
    treeset.h)

# =============================================================================
# Lists of headers populated using check_include_files
# =============================================================================
set(NRN_HEADERS_INCLUDE_LIST)

# =============================================================================
# Lists of random number related files
# =============================================================================
set(RAN_FILE_LIST isaac64.cpp mcran4.cpp nrnisaac.cpp nrnran123.cpp)

# =============================================================================
# Files in oc directory
# =============================================================================
set(OC_FILE_LIST
    ${RAN_FILE_LIST}
    audit.cpp
    axis.cpp
    code.cpp
    code2.cpp
    debug.cpp
    fileio.cpp
    fmenu.cpp
    ftime.cpp
    functabl.cpp
    getsym.cpp
    hoc.cpp
    hocusr.cpp
    hoc_init.cpp
    hoc_oop.cpp
    isoc99.cpp
    list.cpp
    math.cpp
    mswinprt.cpp
    nonlin.cpp
    nrnfilewrap.cpp
    ocerf.cpp
    parallel.cpp
    plot.cpp
    plt.cpp
    regexp.cpp
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
    eion.cpp
    extcelln.cpp
    fadvance.cpp
    fstim.cpp
    hocprax.cpp
    init.cpp
    ldifus.cpp
    nrnnemo.cpp
    nrntimeout.cpp
    nrnversion.cpp
    nrnversion.h
    passive0.cpp
    point.cpp
    psection.cpp
    seclist.cpp
    secref.cpp
    solve.cpp
    synapse.cpp
    treeset.cpp
    multicore.cpp)

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
    cachevec.cpp
    classreg.cpp
    cxprop.cpp
    datapath.cpp
    finithnd.cpp
    geometry3d.cpp
    glinerec.cpp
    hocmech.cpp
    impedanc.cpp
    kschan.cpp
    kssingle.cpp
    linmod.cpp
    linmod1.cpp
    matrixmap.cpp
    multisplit.cpp
    ndatclas.cpp
    netpar.cpp
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
    singlech.cpp
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

# meschach matrix sources
set(MESCH_FILES_LIST
    arnoldi.c
    bdfactor.c
    bkpfacto.c
    chfactor.c
    arnoldi.c
    bdfactor.c
    bkpfacto.c
    chfactor.c
    conjgrad.c
    copy.c
    dmacheps.c
    err.c
    extras.c
    fft.c
    givens.c
    hessen.c
    hsehldr.c
    init.c
    iter0.c
    iternsym.c
    itersym.c
    ivecop.c
    lanczos.c
    lufactor.c
    machine.c
    matlab.c
    matop.c
    matrixio.c
    meminfo.c
    memory.c
    memstat.c
    mfunc.c
    norm.c
    otherio.c
    pxop.c
    qrfactor.c
    schur.c
    solve.c
    sparse.c
    sparseio.c
    spbkp.c
    spchfctr.c
    splufctr.c
    sprow.c
    spswap.c
    submat.c
    svd.c
    symmeig.c
    update.c
    vecop.c
    version.c
    zcopy.c
    zfunc.c
    zgivens.c
    zhessen.c
    zhsehldr.c
    zlufctr.c
    zmachine.c
    zmatio.c
    zmatlab.c
    zmatop.c
    zmemory.c
    znorm.c
    zqrfctr.c
    zschur.c
    zsolve.c
    zvecop.c)

set(SPARSE_FILES_LIST bksub.cpp getelm.cpp lineq.cpp prmat.cpp subrows.cpp)

# sparse13 matrix sources
set(SPARSE13_FILES_LIST
    spalloc.c
    spbuild.c
    spfactor.c
    spoutput.c
    spsolve.c
    sputils.c
    cspalloc.c
    cspbuild.c
    cspfactor.c
    cspoutput.c
    cspsolve.c
    csputils.c)

# scopmath sources
set(SCOPMATH_FILES_LIST
    abort.c
    adams.c
    adeuler.c
    adrunge.c
    advance.c
    boundary.c
    crank.c
    crout.c
    csoda.c
    csodabnc.c
    deflate.c
    dimplic.c
    scoperf.c
    euler.c
    expfit.c
    exprand.c
    f2cmisc.c
    factoria.c
    force.c
    gauss.c
    gear.c
    getmem.c
    harmonic.c
    heun.c
    hyperbol.c
    invert.c
    lag.c
    legendre.c
    newton.c
    normrand.c
    perpulse.c
    perstep.c
    poisrand.c
    poisson.c
    praxis.c
    pulse.c
    quad.c
    ramp.c
    revhyper.c
    revsawto.c
    revsigmo.c
    romberg.c
    runge.c
    sawtooth.c
    schedule.c
    seidel.c
    sigmoid.c
    simeq.c
    simplex.c
    sparse.c
    vsparse.c
    spline.c
    squarewa.c
    ssimplic.c
    step.c
    threshol.c
    tridiag.c
    sparse_thread.c
    newton_thread.c
    crout_thread.c
    ssimplic_thread.c)

set(NRNMPI_FILES_LIST nrnmpi.cpp bbsmpipack.cpp mpispike.cpp)

set(NRNGNU_FILES_LIST
    ACG.cpp
    Binomial.cpp
    DiscUnif.cpp
    Erlang.cpp
    Geom.cpp
    HypGeom.cpp
    LogNorm.cpp
    MLCG.cpp
    NegExp.cpp
    Normal.cpp
    Poisson.cpp
    RNG.cpp
    Random.cpp
    RndInt.cpp
    Uniform.cpp
    Weibull.cpp)

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
    feature
    intfire2
    oclmp
    ppmark
    syn
    exp2syn
    hh
    intfire4
    passive
    stim
    vclmp
    expsyn
    intfire1
    netstim
    pattern
    svclmp)

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

set(IVOS_FILES_LIST listimpl.cpp string.cpp observe.cpp regexp.cpp resource.cpp)

set(MPI_DYNAMIC_INCLUDE nrnmpi_dynam.h nrnmpi_dynam_cinc nrnmpi_dynam_wrappers.inc)

# =============================================================================
# Top level directories under src
# =============================================================================
set(NRN_OC_SRC_DIR ${PROJECT_SOURCE_DIR}/src/oc)
set(NRN_NRNOC_SRC_DIR ${PROJECT_SOURCE_DIR}/src/nrnoc)
set(NRN_IVOC_SRC_DIR ${PROJECT_SOURCE_DIR}/src/ivoc)
set(NRN_NRNCVODE_SRC_DIR ${PROJECT_SOURCE_DIR}/src/nrncvode)
set(NRN_NRNIV_SRC_DIR ${PROJECT_SOURCE_DIR}/src/nrniv)
set(NRN_MODLUNIT_SRC_DIR ${PROJECT_SOURCE_DIR}/src/modlunit)
set(NRN_NMODL_SRC_DIR ${PROJECT_SOURCE_DIR}/src/nmodl)
set(NRN_IVOS_SRC_DIR ${PROJECT_SOURCE_DIR}/src/ivos)

# =============================================================================
# Create source file lists by gathering from various directories
# =============================================================================
nrn_create_file_list(NRN_OC_SRC_FILES ${NRN_OC_SRC_DIR} ${OC_FILE_LIST})
nrn_create_file_list(NRN_NRNOC_SRC_FILES ${NRN_NRNOC_SRC_DIR} ${NRNOC_FILE_LIST})
nrn_create_file_list(NRN_IVOC_SRC_FILES ${NRN_IVOC_SRC_DIR} ${IVOC_FILE_LIST})
nrn_create_file_list(NRN_NRNCVODE_SRC_FILES ${NRN_NRNCVODE_SRC_DIR} ${NRNCVODE_FILE_LIST})
nrn_create_file_list(NRN_NRNIV_SRC_FILES ${NRN_NRNIV_SRC_DIR} ${NRNIV_FILE_LIST})
nrn_create_file_list(NRN_PARALLEL_SRC_FILES ${PROJECT_SOURCE_DIR}/src/nrniv
                     nvector_nrnparallel_ld.cpp)
nrn_create_file_list(NRN_PARALLEL_SRC_FILES ${PROJECT_SOURCE_DIR}/src/sundials/shared
                     nvector_parallel.c)
nrn_create_file_list(NRN_MESCH_SRC_FILES ${PROJECT_SOURCE_DIR}/src/mesch ${MESCH_FILES_LIST})
nrn_create_file_list(NRN_SPARSE_SRC_FILES ${PROJECT_SOURCE_DIR}/src/sparse ${SPARSE_FILES_LIST})
nrn_create_file_list(NRN_SPARSE13_SRC_FILES ${PROJECT_SOURCE_DIR}/src/sparse13
                     ${SPARSE13_FILES_LIST})
nrn_create_file_list(NRN_SCOPMATH_SRC_FILES ${PROJECT_SOURCE_DIR}/src/scopmath
                     ${SCOPMATH_FILES_LIST})
nrn_create_file_list(NRN_NRNMPI_SRC_FILES ${PROJECT_SOURCE_DIR}/src/nrnmpi ${NRNMPI_FILES_LIST})
nrn_create_file_list(NRN_NRNGNU_SRC_FILES ${PROJECT_SOURCE_DIR}/src/gnu ${NRNGNU_FILES_LIST})
nrn_create_file_list(NRN_NRNPYTHON_SRC_FILES ${PROJECT_SOURCE_DIR}/src/nrnpython
                     ${NRNPYTHON_FILES_LIST})
nrn_create_file_list(NRN_MODFILE_BASE_NAMES ${PROJECT_SOURCE_DIR}/src/nrnoc ${MODFILE_BASE_NAMES})
nrn_create_file_list(NRN_BIN_SRC_FILES ${PROJECT_SOURCE_DIR}/src/ivoc/ nrnmain.cpp)
nrn_create_file_list(NRN_BIN_SRC_FILES ${PROJECT_SOURCE_DIR}/src/oc/ ockludge.cpp modlreg.cpp)
nrn_create_file_list(NRN_MODLUNIT_SRC_FILES ${NRN_MODLUNIT_SRC_DIR} ${MODLUNIT_FILES_LIST})
nrn_create_file_list(NRN_NMODL_SRC_FILES ${NRN_NMODL_SRC_DIR} ${NMODL_FILES_LIST})
nrn_create_file_list(NRNMPI_DYNAMIC_INCLUDE_FILE ${PROJECT_SOURCE_DIR}/src/nrnmpi
                     ${MPI_DYNAMIC_INCLUDE})
nrn_create_file_list(NRN_IVOS_SRC_FILES ${NRN_IVOS_SRC_DIR} ${IVOS_FILES_LIST})
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
