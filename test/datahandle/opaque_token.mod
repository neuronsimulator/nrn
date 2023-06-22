COMMENT
  In mechanism libraries, cannot use
      auto const token = nrn_ensure_model_data_are_sorted();
  because the reference is incomplete (from include/neuron/model_data_fwd.hpp).
  So use an opaque version of the reference if need to call a function with
  an argument of type neuron::model_sorted_token const&
ENDCOMMENT

NEURON { SUFFIX nothing }

PROCEDURE opaque_advance(){
  VERBATIM
    auto const token = nrn_ensure_model_data_are_sorted_opaque();
    nrn_fixed_step(token);
  ENDVERBATIM
}
