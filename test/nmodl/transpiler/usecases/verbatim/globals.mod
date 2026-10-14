NEURON {
  SUFFIX globals
  GLOBAL gbl
}

ASSIGNED {
  gbl
}

FUNCTION get_gbl() {
VERBATIM
_lget_gbl = gbl;
ENDVERBATIM
}
