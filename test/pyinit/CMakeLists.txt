# Make sure that exit codes are propagated as expected, and that the expected number of arguments is
# processed. Start out with checking that the right amount of code is executed, and that error codes
# are propagated. Here, we don't bother to duplicate for all explicit/implicit Python versions.
nrn_add_test_group(
  NAME pyinit
  MODFILE_PATTERNS NONE # no special, for now
  SIM_DIRECTORY test/pyinit)
set(nrniv_args_common -notatty)
set(nrniv_args ${nrniv_args_common})
# Passing a .py file without -python takes a different code path to passing a .py file *with*
# -python
nrn_add_test(
  GROUP pyinit
  NAME nrniv_script.py
  COMMAND nrniv ${nrniv_args} do_nothing.py
  SCRIPT_PATTERNS do_nothing.py)
set_property(TEST pyinit::nrniv_script.py PROPERTY PASS_REGULAR_EXPRESSION
                                                   "[\r\n]doing nothing[\r\n]")
# If the script returns an error code, nrniv should return that code too
nrn_add_test(
  GROUP pyinit
  NAME nrniv_script.py_error
  COMMAND nrniv ${nrniv_args} assert_false.py
  SCRIPT_PATTERNS assert_false.py)
set_property(TEST pyinit::nrniv_script.py_error PROPERTY WILL_FAIL ON)
# Passing two .py files *without* -python executes both of them, but that behaviour is not really
# documented and should perhaps be deprecated due to ambiguity about how to match python's semantics
# for sys.path.
nrn_add_test(
  GROUP pyinit
  NAME nrniv_two_scripts.py
  COMMAND nrniv ${nrniv_args} do_nothing.py do_nothing.py
  SCRIPT_PATTERNS do_nothing.py)
set_property(TEST pyinit::nrniv_two_scripts.py
             PROPERTY PASS_REGULAR_EXPRESSION "[\r\n]doing nothing[\r\n]doing nothing[\r\n]")
# Unfortunately we can't easily add CTest tests that check exit codes *and* regex output. So we just
# run the tests twice.
foreach(check code output)
  nrn_add_test(
    GROUP pyinit
    NAME nrniv_two_scripts.py_error_check_${check}
    COMMAND nrniv ${nrniv_args} do_nothing.py assert_false.py
    SCRIPT_PATTERNS do_nothing.py assert_false.py)
  if(check STREQUAL code)
    set(property WILL_FAIL ON)
  else()
    set(property PASS_REGULAR_EXPRESSION "[\r\n]doing nothing[\r\n]")
  endif()
  set_property(TEST pyinit::nrniv_two_scripts.py_error_check_${check} PROPERTY ${property})
endforeach()
# When we pass -python then we have the option of passing a .py script file or a -c command; first
# test -c
nrn_add_test(
  GROUP pyinit
  NAME nrniv_python_command
  COMMAND nrniv ${nrniv_args} -python -c "print('foo')")
set_property(TEST pyinit::nrniv_python_command PROPERTY PASS_REGULAR_EXPRESSION "[\r\n]foo[\r\n]")
nrn_add_test(
  GROUP pyinit
  NAME nrniv_python_command_error
  COMMAND nrniv ${nrniv_args} -python -c "assert False")
set_property(TEST pyinit::nrniv_python_command_error PROPERTY WILL_FAIL ON)
# Then test -python foo.py
nrn_add_test(
  GROUP pyinit
  NAME nrniv_python_script.py
  COMMAND nrniv ${nrniv_args} -python do_nothing.py
  SCRIPT_PATTERNS do_nothing.py)
set_property(TEST pyinit::nrniv_python_script.py PROPERTY PASS_REGULAR_EXPRESSION
                                                          "[\r\n]doing nothing[\r\n]")
nrn_add_test(
  GROUP pyinit
  NAME nrniv_python_script.py_error
  COMMAND nrniv ${nrniv_args} -python assert_false.py
  SCRIPT_PATTERNS assert_false.py)
set_property(TEST pyinit::nrniv_python_script.py_error PROPERTY WILL_FAIL ON)
# With -python then NEURON has the same semantics as python, i.e only one file.py or -c "code"
# argument is processed and the remainder is just passed as arguments. So asserting False in a
# second -c should not cause a non-zero exit code.
nrn_add_test(
  GROUP pyinit
  NAME nrniv_python_command_twice
  COMMAND nrniv ${nrniv_args} -python -c "print('hello')" -c "assert False")
set_property(TEST pyinit::nrniv_python_command_twice PROPERTY PASS_REGULAR_EXPRESSION
                                                              "[\r\n]hello[\r\n]")
nrn_add_test(
  GROUP pyinit
  NAME nrniv_python_script.py_twice
  COMMAND nrniv ${nrniv_args} -python do_nothing.py assert_false.py
  SCRIPT_PATTERNS do_nothing.py assert_false.py)
set_property(TEST pyinit::nrniv_python_script.py_twice PROPERTY PASS_REGULAR_EXPRESSION
                                                                "[\r\n]doing nothing[\r\n]")
string(
  JOIN
  "$<SEMICOLON> "
  python_cmd
  "import sys"
  "actual = sys.argv[-2:]"
  "print(actual)"
  "assert actual == ['foo', 'bar']")
nrn_add_test(
  GROUP pyinit
  NAME nrniv_python_command_args
  COMMAND nrniv ${nrniv_args} -python -c "${python_cmd}" foo bar)
nrn_add_test(
  GROUP pyinit
  NAME nrniv_python_script.py_args
  COMMAND nrniv ${nrniv_args} -python assert_last_args_are_foo_and_bar.py foo bar
  SCRIPT_PATTERNS assert_last_args_are_foo_and_bar.py)
# With -nopython then there should be a dummy PythonObject implementation, and Python-related
# settings should be ignored
set(PythonObject_cmd -c "objref x" -c "x=new PythonObject()")
nrn_add_test(
  GROUP pyinit
  NAME nrniv_nopython_PythonObject
  COMMAND nrniv ${nrniv_args} -nopython ${PythonObject_cmd})
nrn_add_test(
  GROUP pyinit
  NAME nrniv_nopython_PythonObject_pyexe
  COMMAND nrniv ${nrniv_args} -nopython -pyexe /deep/thought ${PythonObject_cmd})
nrn_add_test(
  GROUP pyinit
  NAME nrniv_nopython_PythonObject_pyenv
  COMMAND NRN_PYTHONVERSION=life NRN_PYLIB=the-universe NRN_PYTHONEXE=and-everything nrniv
          ${nrniv_args} -nopython ${PythonObject_cmd})
#
# * nrniv -pyexe /path/to/python -python ...
# * NRN_PYTHONEXE=... nrniv -python ...
# * PATH=nothing/with/python/init nrniv -python ...
# * python something_importing_neuron.py
# * python -c "import neuron; other stuff"
# * nrniv script_using_multiprocessing.py
# * special -python -c "import neuron" [where /path/to/special = {venv_root}/some_name/special]
#
# plus variants of the above when a virtual environment is active, or an explicit path to a virtual
# environment python is given.
#
# these should assert that the variants launched with `python` and `nrniv` end up with consistent
# values of `sys.path`, `sys.[base_][exec_]prefix`, `sys.std{err,in,out}` encoding and Python
# version number. If we are building (with dynamic Python) for multiple Python versions, replicate
# the tests for all of them. TODO use NRN_PYTHON_EXTRA_FOR_TESTS.
foreach(val RANGE ${NRN_PYTHON_COUNT})
  # val = 0 .. NRN_PYTHON_COUNT-1 means explicitly use that Python; val = NRN_PYTHON_COUNT means
  # don't specify and check we get the expected default.
  set(nrniv_args ${nrniv_args_common})
  if(val EQUAL NRN_PYTHON_COUNT)
    # Figure out which Python we expect nrniv -python to pick up in these tests. First, for dynamic
    # Python:
    #
    # * we know we're not passing -pyexe here
    # * we know(?) that NRN_PYTHONEXE is not set
    # * the order of precedence is: python python3 pythonX0.Y0 .. pythonXn.Yn
    # * disable the tests if that yields nothing
    #
    # python3.999 is to make sure we test skipping names in the search path that don't exist.
    # Secondly, if dynamic Python is disabled, then we know it will use the default.
    if(NRN_ENABLE_PYTHON_DYNAMIC)
      set(search_names python python3 python3.999)
      foreach(val2 RANGE ${NRN_PYTHON_ITERATION_LIMIT})
        list(GET NRN_PYTHON_VERSIONS ${val2} pyver)
        list(APPEND search_names python${pyver})
      endforeach()
      unset(pyexe)
      foreach(candidate ${search_names})
        nrn_find_python(NAME "${candidate}" PREFIX nrnpy)
        if(nrnpy_EXECUTABLE STREQUAL "nrnpy_EXECUTABLE-NOTFOUND")
          # Not an error if a user's system doesn't have all the names in ${search_names}
          continue()
        endif()
        set(nrnpy_VERSION "${nrnpy_VERSION_MAJOR}.${nrnpy_VERSION_MINOR}")
        if(NOT nrnpy_VERSION IN_LIST NRN_PYTHON_VERSIONS)
          # e.g. python or python3 points to a version we didn't build against
          continue()
        endif()
        # Right, so the default should be nrnpy_EXECUTABLE, which is nrnpy_VERSION
        set(pyexe "${nrnpy_EXECUTABLE}")
        set(pyver "${nrnpy_VERSION}")
        break()
      endforeach()
    else()
      # non-dynamic Python, so nrniv -python will always use this. -pyexe in this case just steers
      # the value of sys.executable, but we're not passing that here anyway.
      set(pyexe "${NRN_DEFAULT_PYTHON_EXECUTABLE}")
      set(pyver "${NRN_DEFAULT_PYTHON_VERSION}")
    endif()
    if(NOT DEFINED pyexe)
      message(
        WARNING "Couldn't figure out what Python version nrniv -python will choose by default")
      # Skip these tests
      continue()
    endif()
    set(pyexe_name "def")
  else()
    list(GET NRN_PYTHON_EXECUTABLES ${val} pyexe)
    list(GET NRN_PYTHON_VERSIONS ${val} pyver)
    list(APPEND nrniv_args -pyexe "${pyexe}")
    set(pyexe_name "${pyver}")
  endif()
  set(nrnivpy nrniv_py${pyexe_name})
  # Now move on to checking things about which Python is actually used. First, make sure the version
  # matches our expectation.
  string(
    JOIN
    "$<SEMICOLON> "
    python_cmd
    "import sys"
    "actual = '{}.{}'.format(*sys.version_info[:2])"
    "print(actual)"
    "assert actual == '${pyver}'")
  nrn_add_test(
    GROUP pyinit
    NAME ${nrnivpy}_python_command_version_check
    COMMAND nrniv ${nrniv_args} -python -c "${python_cmd}")
  # Now check that some other Python things match between `nrniv -python` and `${exe}` directly
  foreach(
    attr
    # executable # seems tricky to get exact matches portably, maybe replace with a looser "they all
    # look like python not nrniv/special" check
    path
    prefix
    exec_prefix
    base_prefix
    base_exec_prefix
    stderr.encoding
    stdin.encoding
    stdout.encoding)
    # Make sure that sys.${attr} matches when we run `python a.py` and `nrniv -python a.py`
    nrn_add_test(
      GROUP pyinit
      NAME ${nrnivpy}_python_check_sys_${attr}
      PRECOMMAND "${pyexe}" dump_sys_attr.py ref.json ${attr}
      COMMAND nrniv ${nrniv_args} -python check_sys_attr.py ref.json ${attr}
      SCRIPT_PATTERNS check_sys_attr.py dump_sys_attr.py)
    # Replicate this with nrnpython(...) inside a .hoc script too. This checks that the Python
    # environment inside nrnpython(...) inside foo.hoc is the same as in foo.py -- which seems like
    # a reasonable choice -- except for path, which has an explicit exception documented in (for
    # now) hoc_moreinput -- there nrnpython(...) inside foo.hoc sees sys.path[0] == '' for backward
    # compatibility reasons.
    nrn_add_test(
      GROUP pyinit
      NAME ${nrnivpy}_nrnpython_check_sys_${attr}
      PRECOMMAND "${pyexe}" dump_sys_attr.py override_sys_path_0_to_be_empty ref.json ${attr}
      COMMAND nrniv ${nrniv_args} -c "strdef attr, fname" -c "attr=\"${attr}\"" -c
              "fname=\"ref.json\"" check_sys_attr.hoc
      SCRIPT_PATTERNS check_sys_attr.hoc dump_sys_attr.py)
  endforeach()
  # Make sure that we can import neuron
  nrn_add_test(
    GROUP pyinit
    NAME ${nrnivpy}_python_command_import_neuron
    COMMAND nrniv ${nrniv_args} -python -c "import neuron")
  nrn_add_test(
    GROUP pyinit
    NAME py${pyexe_name}_command_import_neuron
    COMMAND "${pyexe}" -c "import neuron"
    PRELOAD_SANITIZER)
  # Check using Python from HOC using nrnpython(...). The only reasonable approach here seems to be
  # that sys.path should match the code calling nrnpython(...), i.e.
  #
  # * foo.hoc or foo.py: sys.path[0] should be the directory containing the script, with symlinks
  #   resolved
  # * nrniv -c "hoc_code_including_nrnpython" or nrniv -python -c "from neuron import h;
  #   h.nrnpython(...)": sys.path[0] should be an empty string
  #
  nrn_add_test(
    GROUP pyinit
    NAME ${nrnivpy}_hoc_command_nrnpython
    COMMAND nrniv ${nrniv_args} -c "nrnpython(\"import sys$<SEMICOLON> print(sys.path)\")")
endforeach()
if(NRN_PYTHON_EXTRA_FOR_TESTS)
  # Run some tests using Python versions that are installed on the system, but which NEURON is *not*
  # built with support for.
  foreach(val RANGE ${NRN_PYTHON_EXTRA_FOR_TESTS_ITERATION_LIMIT})
    list(GET NRN_PYTHON_EXTRA_FOR_TESTS_EXECUTABLES ${val} pyexe)
    list(GET NRN_PYTHON_EXTRA_FOR_TESTS_VERSIONS ${val} pyver)
    # As above, we can't easily add CTest tests that check exit codes *and* regex output. So we just
    # run the tests twice.
    foreach(check code output)
      # Should not be able to import NEURON
      nrn_add_test(
        GROUP pyinit
        NAME python${pyver}_import_fail_check_${check}
        COMMAND "${pyexe}" -c "import neuron"
        PRELOAD_SANITIZER)
      if(check STREQUAL code)
        set(properties WILL_FAIL ON)
      else()
        # output
        string(REPLACE "." "\\." pyver_escaped "${pyver}")
        set(properties
            PASS_REGULAR_EXPRESSION
            "Python ${pyver_escaped} is not supported by this NEURON installation \\(supported:")
      endif()
      set_tests_properties(pyinit::python${pyver}_import_fail_check_${check}
                           PROPERTIES ${properties})
      # Telling nrniv to use this Python should fail if dynamic Python is enabled. TODO: figured out
      # the desired behaviour for non-dynamic Python.
      if(NRN_ENABLE_PYTHON_DYNAMIC)
        nrn_add_test(
          GROUP pyinit
          NAME nrniv_py${pyver}_import_fail_check_${check}
          COMMAND nrniv ${nrniv_args_common} -pyexe ${pyexe} -python -c "import neuron")
        # Setting the full set of environment variables (NRN_PYLIB, NRN_PYTHONEXE and
        # NRN_PYTHONVERSION) will prevent nrnpyenv.sh from being run, but we should still get an
        # error if NRN_PYTHONVERSION is not valid for this NEURON.
        execute_process(
          COMMAND "${pyexe}" -c "import sysconfig; print(sysconfig.get_config_var('LIBDIR'))"
          RESULT_VARIABLE code
          OUTPUT_VARIABLE libdir
          OUTPUT_STRIP_TRAILING_WHITESPACE)
        set(pylib
            "${libdir}/${CMAKE_SHARED_LIBRARY_PREFIX}python${pyver}${CMAKE_SHARED_LIBRARY_SUFFIX}")
        if(code OR NOT EXISTS "${pylib}")
          message(
            WARNING
              "Could not identify libpythonX.Y for NRN_PYTHON_EXTRA_FOR_TESTS test (tried ${pylib})"
          )
          continue()
        endif()
        nrn_add_test(
          GROUP pyinit
          NAME nrniv_py${pyver}_forced_from_env_check_${check}
          COMMAND NRN_PYTHONVERSION=${pyver} "NRN_PYLIB=${pylib}" "NRN_PYTHONEXE=${pyexe}" nrniv
                  -python -c "import neuron")
        set_tests_properties(
          pyinit::nrniv_py${pyver}_import_fail_check_${check}
          pyinit::nrniv_py${pyver}_forced_from_env_check_${check} PROPERTIES ${properties})
      endif()
    endforeach()
  endforeach()
endif()
