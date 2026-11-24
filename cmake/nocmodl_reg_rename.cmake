# nocmodl writes `void _hh_reg()`. src/nrnoc/cabvars.h declares `_hh_reg_()`. The old step was `sed
# s/_reg()/_reg_()/`. sed is not on the documented MSVC toolchain (cmake/generate_hocusr.cmake).
# Each registration is that token once, on its own line. Other calls pass arguments
# (`_nrn_thread_reg(int, ...)`), so they are not `_reg()`.

if(NOT DEFINED NRN_NOCMODL_CPP OR NRN_NOCMODL_CPP STREQUAL "")
  message(FATAL_ERROR "nocmodl_reg_rename.cmake: missing -DNRN_NOCMODL_CPP")
endif()

file(READ "${NRN_NOCMODL_CPP}" _nrn_nocmodl_src)
string(REPLACE "_reg()" "_reg_()" _nrn_nocmodl_src "${_nrn_nocmodl_src}")
file(WRITE "${NRN_NOCMODL_CPP}" "${_nrn_nocmodl_src}")
