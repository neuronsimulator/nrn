# cmake-format: off
# ===============================================================
# The NEURON Python module
# The module used to be installed by setuptools, but now we do it manually
# since scikit-build-core (the new build backend) is not flexible enough for
# our needs
# ===============================================================
# cmake-format: on

set(NRN_PYTHON_FILES_LIST
    hclass3.py
    expect_hocerr.py
    psection.py
    neuroml/metadata.py
    neuroml/morphml.py
    neuroml/xml2nrn.py
    neuroml/__init__.py
    neuroml/neuroml.py
    neuroml/rdxml.py
    neuroml/biophysics.py
    config.py
    units.py
    sections.py
    tests/_subclass.py
    tests/test_all.py
    tests/__init__.py
    tests/utils/coreneuron_available.py
    tests/utils/__init__.py
    tests/utils/strtobool.py
    tests/utils/capture_stdout.py
    tests/utils/checkresult.py
    tests/test_neuron.py
    tests/test_vector.py
    __init__.py
    coreneuron.py
    doc.py
    gui2/setup_threejs.py
    gui2/config.py
    gui2/__init__.py
    gui2/rangevar.py
    gui2/plotshape.py
    gui2/utilities.py
    crxd/__init__.py
    gui.py
    nonvint_block_supervisor.py)

set(NRN_PYTHON_NMODL_FILES_LIST
    nmodl/dsl.py
    nmodl/symtab.py
    nmodl/ext/example/passive.mod
    nmodl/ext/example/expsyn.mod
    nmodl/ext/example/exp2syn.mod
    nmodl/ext/example/hh.mod
    nmodl/ext/viz/index.html
    nmodl/ext/viz/css/tree.css
    nmodl/ext/viz/js/d3.min.js
    nmodl/ext/viz/js/tree.js
    nmodl/__init__.py
    nmodl/visitor.py
    nmodl/ast.py
    nmodl/ode.py)

set(NRN_PYTHON_RX3D_FILES_LIST
    rxd/rxdException.py
    rxd/options.py
    rxd/plugins.py
    rxd/rxdsection.py
    rxd/species.py
    rxd/rxdmath.py
    rxd/rate.py
    rxd/region.py
    rxd/multiCompartmentReaction.py
    rxd/constants.py
    rxd/dimension3.py
    rxd/__init__.py
    rxd/section1d.py
    rxd/export.py
    rxd/initializer.py
    rxd/rangevar.py
    rxd/geometry.py
    rxd/rxd.py
    rxd/generalizedReaction.py
    rxd/nodelist.py
    rxd/node.py
    rxd/geometry3d/simplevolume_helper.py
    rxd/geometry3d/GeneralizedVoxelization.py
    rxd/geometry3d/FullJoinMorph.py
    rxd/geometry3d/scalarField.py
    rxd/geometry3d/__init__.py
    rxd/geometry3d/surface_a.py
    rxd/geometry3d/triangularMesh.py
    rxd/geometry3d/voxelize.py
    rxd/geometry3d/voxelize2.py
    rxd/geometry3d/surface.py
    rxd/gui.py
    rxd/reaction.py
    rxdtests/tests/ca_pump.py
    rxdtests/tests/3d/pure_diffusion_3d_cvode.py
    rxdtests/tests/3d/circadian_rhythm.py
    rxdtests/tests/3d/include_flux3d.py
    rxdtests/tests/3d/pure_diffusion_3d.py
    rxdtests/tests/3d/multicompartment_reactions.py
    rxdtests/tests/3d/include_flux3d_cvode.py
    rxdtests/tests/3d/torun.txt
    rxdtests/tests/multicompartment_mebrane_mismatch.py
    rxdtests/tests/wave1d/wave1d_125_1.py
    rxdtests/tests/wave1d/wave1d_tree2_125_1.py
    rxdtests/tests/wave1d/wave1d_tree2_25_5.py
    rxdtests/tests/wave1d/caleak.mod
    rxdtests/tests/wave1d/wave1d_tree_125_1.py
    rxdtests/tests/wave1d/wave1d_taper_25_5.py
    rxdtests/tests/wave1d/readme.txt
    rxdtests/tests/wave1d/torun.txt
    rxdtests/tests/wave1d/wave1d_25_5.py
    rxdtests/tests/wave1d/wave1d_taper_125_1.py
    rxdtests/tests/wave1d/wave1d_tree_25_5.py
    rxdtests/tests/wave1d/cal2.mod
    rxdtests/tests/multicompartment_reactions_with_v.py
    rxdtests/tests/nodes_update.py
    rxdtests/tests/c91662.swc
    rxdtests/tests/hh_param.py
    rxdtests/tests/cabuf.py
    rxdtests/tests/ecs/ecs_include_flux_cvode.py
    rxdtests/tests/ecs/ecs_before_sections.py
    rxdtests/tests/ecs/ecs_example_cvode.py
    rxdtests/tests/ecs/ecs_multi_example.py
    rxdtests/tests/ecs/ecs_multi_example_cvode.py
    rxdtests/tests/ecs/pump.mod
    rxdtests/tests/ecs/torun.txt
    rxdtests/tests/ecs/ecs_example.py
    rxdtests/tests/ecs/ecs_include_flux.py
    rxdtests/tests/hh_cvode.py
    rxdtests/tests/include_flux_cvode.py
    rxdtests/tests/hh_param_cvode.py
    rxdtests/tests/cabuf_fixed_step.py
    rxdtests/tests/pure_diffusion_cvode.py
    rxdtests/tests/hh_morph.py
    rxdtests/tests/IraHH.py
    rxdtests/tests/react_region_specified.py
    rxdtests/tests/include_flux.py
    rxdtests/tests/verify_no_initialization_order_issue.py
    rxdtests/tests/hh.py
    rxdtests/tests/reaction_test.py
    rxdtests/tests/multicompartment_reactions_del.py
    rxdtests/tests/reaction_param_test.py
    rxdtests/tests/multicompartment_reactions.py
    rxdtests/tests/pure_diffusion.py
    rxdtests/tests/torun.txt
    rxdtests/tests/reaction_null_dest.py
    rxdtests/tests/hybrid/pure_diffusion_hybrid_cvode.py
    rxdtests/tests/hybrid/bistable_hybrid_cvode.py
    rxdtests/tests/hybrid/bistable_hybrid_change_nthread.py
    rxdtests/tests/hybrid/pure_diffusion_hybrid.py
    rxdtests/tests/hybrid/bistable_hybrid_cvode_change_nthread.py
    rxdtests/tests/hybrid/bistable_hybrid.py
    rxdtests/tests/hybrid/torun.txt
    rxdtests/do_test.py
    rxdtests/readme.txt
    rxdtests/run_all.py
    tests/test_rxd.py
    tests/test_rxd.json)

foreach(pyfile IN LISTS NRN_PYTHON_FILES_LIST)
  get_filename_component(pyfile_abs "${CMAKE_CURRENT_SOURCE_DIR}/${pyfile}" ABSOLUTE)
  get_filename_component(pyfile_dir "${pyfile}" DIRECTORY)
  install(FILES "${pyfile_abs}" DESTINATION "${NRN_WHEEL_INSTALL_PREFIX}/${pyfile_dir}")
endforeach()

if(NRN_ENABLE_RX3D)
  foreach(pyfile IN LISTS NRN_PYTHON_RX3D_FILES_LIST)
    get_filename_component(pyfile_abs "${CMAKE_CURRENT_SOURCE_DIR}/${pyfile}" ABSOLUTE)
    get_filename_component(pyfile_dir "${pyfile}" DIRECTORY)
    install(FILES "${pyfile_abs}" DESTINATION "${NRN_WHEEL_INSTALL_PREFIX}/${pyfile_dir}")
  endforeach()
endif()

if(NRN_ENABLE_CORENEURON OR NRN_ENABLE_NMODL)
  foreach(pyfile IN LISTS NRN_PYTHON_NMODL_FILES_LIST)
    get_filename_component(pyfile_abs "${CMAKE_CURRENT_SOURCE_DIR}/${pyfile}" ABSOLUTE)
    get_filename_component(pyfile_dir "${pyfile}" DIRECTORY)
    install(FILES "${pyfile_abs}" DESTINATION "${NRN_WHEEL_INSTALL_PREFIX}/${pyfile_dir}")
  endforeach()

endif()
