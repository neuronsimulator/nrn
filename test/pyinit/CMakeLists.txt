# Make sure that exit codes are propagated as expected, and that the expected number of arguments is
# processed. Start out with checking that the right amount of code is executed, and that error codes
# are propagated. Here, we don't bother to duplicate for all explicit/implicit Python versions.s
nrn_add_test_group(
  NAME pyinit
  MODFILE_PATTERNS NONE
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
nrn_add_test(
  GROUP pyinit
  NAME nrniv_two_scripts.py_error
  COMMAND nrniv ${nrniv_args} do_nothing.py assert_false.py
  SCRIPT_PATTERNS exit_0.py assert_false.py)
set_property(TEST pyinit::nrniv_two_scripts.py_error PROPERTY PASS_REGULAR_EXPRESSION
                                                              "[\r\n]doing nothing[\r\n]")
set_property(TEST pyinit::nrniv_two_scripts.py_error PROPERTY WILL_FAIL ON)
# When we pass -python then we have the option of passing a .py script file or a -c command; first
# test -c
nrn_add_test(
  GROUP pyinit
  NAME nrniv_python_command
  COMMAND nrniv ${nrniv_args} -python -c "print('foo')"
  SCRIPT_PATTERNS)
set_property(TEST pyinit::nrniv_python_command PROPERTY PASS_REGULAR_EXPRESSION "[\r\n]foo[\r\n]")
nrn_add_test(
  GROUP pyinit
  NAME nrniv_python_command_error
  COMMAND nrniv ${nrniv_args} -python -c "assert False"
  SCRIPT_PATTERNS)
set_property(
  TEST pyinit::nrniv_python_command_error
  PROPERTY
  PROPERTY WILL_FAIL ON)
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
  COMMAND nrniv ${nrniv_args} -python -c "print('hello')" -c "assert False"
  SCRIPT_PATTERNS)
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
  COMMAND nrniv ${nrniv_args} -python -c "${python_cmd}" foo bar
  SCRIPT_PATTERNS)
nrn_add_test(
  GROUP pyinit
  NAME nrniv_python_script.py_args
  COMMAND nrniv ${nrniv_args} -python assert_last_args_are_foo_and_bar.py foo bar
  SCRIPT_PATTERNS assert_last_args_are_foo_and_bar.py)
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
# the tests for all of them. Add these tests to the pynrn group so that a custom special is
# available as well as plain nrniv.
foreach(val RANGE ${NRN_PYTHON_COUNT})
  # val = 0 .. NRN_PYTHON_COUNT-1 means explicitly use that Python; val = NRN_PYTHON_COUNT means
  # don't specify and check we get the expected default.
  set(nrniv_args ${nrniv_args_common})
  if(val EQUAL NRN_PYTHON_COUNT)
    list(GET NRN_PYTHON_EXECUTABLES 0 exe)
    list(GET NRN_PYTHON_VERSIONS 0 pyver)
    set(pyexe_name "def")
    # TODO figure out what version we expect to pick up by default; with dynamic Python this depends
    # on the runtime $PATH
  else()
    list(GET NRN_PYTHON_EXECUTABLES ${val} exe)
    list(GET NRN_PYTHON_VERSIONS ${val} pyver)
    list(APPEND nrniv_args -pyexe "${exe}")
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
    COMMAND nrniv ${nrniv_args} -python -c "${python_cmd}"
    SCRIPT_PATTERNS)
  # Now check that some other Python things match between `nrniv -python` and `${exe}` directly
  foreach(
    attr
    executable
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
      PRECOMMAND "${exe}" dump_sys_attr.py ref.json ${attr}
      COMMAND nrniv ${nrniv_args} -python check_sys_attr.py ref.json ${attr}
      SCRIPT_PATTERNS check_sys_attr.py dump_sys_attr.py)
  endforeach()
endforeach()
