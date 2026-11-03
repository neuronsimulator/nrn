# Preprocess src/nrnoc/neuron.h and run mk_hocusr_h.py. cl.exe treats a leading //UNC path as a
# switch, and sed is not a Windows program. Copy neuron.h next to the output, preprocess the local
# name, feed stdout to Python (which skips leftover #line markers).
#
# -DNRN_HOCUSR_CC, _NRNOC, _OC, _NEURON_H, _PYTHON, _SCRIPT, _OUTPUT, _WORKDIR Optional:
# NRN_HOCUSR_NOLOGO=1 for MSVC.

if(NOT NRN_HOCUSR_CC
   OR NOT NRN_HOCUSR_NRNOC
   OR NOT NRN_HOCUSR_OC
   OR NOT NRN_HOCUSR_NEURON_H
   OR NOT NRN_HOCUSR_PYTHON
   OR NOT NRN_HOCUSR_SCRIPT
   OR NOT NRN_HOCUSR_OUTPUT
   OR NOT NRN_HOCUSR_WORKDIR)
  message(FATAL_ERROR "generate_hocusr.cmake: missing -DNRN_HOCUSR_*")
endif()

file(MAKE_DIRECTORY "${NRN_HOCUSR_WORKDIR}")
file(COPY "${NRN_HOCUSR_NEURON_H}" DESTINATION "${NRN_HOCUSR_WORKDIR}")

set(_nrn_hocusr_nologo)
if(NRN_HOCUSR_NOLOGO)
  set(_nrn_hocusr_nologo -nologo)
endif()

execute_process(
  COMMAND "${NRN_HOCUSR_CC}" -E ${_nrn_hocusr_nologo} "-I${NRN_HOCUSR_NRNOC}" "-I${NRN_HOCUSR_OC}"
          neuron.h
  WORKING_DIRECTORY "${NRN_HOCUSR_WORKDIR}"
  OUTPUT_FILE "${NRN_HOCUSR_WORKDIR}/neuron.tmp1"
  RESULT_VARIABLE _nrn_hocusr_rv
  ERROR_VARIABLE _nrn_hocusr_err)
if(_nrn_hocusr_rv)
  message(FATAL_ERROR "preprocess neuron.h failed (${_nrn_hocusr_rv}): ${_nrn_hocusr_err}")
endif()

execute_process(
  COMMAND "${NRN_HOCUSR_PYTHON}" "${NRN_HOCUSR_SCRIPT}"
  INPUT_FILE "${NRN_HOCUSR_WORKDIR}/neuron.tmp1"
  OUTPUT_FILE "${NRN_HOCUSR_OUTPUT}"
  WORKING_DIRECTORY "${NRN_HOCUSR_WORKDIR}"
  RESULT_VARIABLE _nrn_hocusr_rv
  ERROR_VARIABLE _nrn_hocusr_err)
if(_nrn_hocusr_rv)
  message(FATAL_ERROR "mk_hocusr_h.py failed (${_nrn_hocusr_rv}): ${_nrn_hocusr_err}")
endif()
