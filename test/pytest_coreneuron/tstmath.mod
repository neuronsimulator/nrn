NEURON {
  SUFFIX tstmath
  THREADSAFE
}

FUNCTION div(x, y) {
  div = x/y
}

FUNCTION root2(x) {
  root2 = sqrt(x)
}

FUNCTION natlog(x) {
  natlog = log(x)
}

FUNCTION expon(x) {
  expon = 0.0
VERBATIM
  // use the system version of exp.
  #ifdef exp
  #undef exp
  #define UNDEFEXP
  #endif
  return exp(_lx);
  #ifdef UNDEFEXP
  #define exp hoc_Exp
  #undef UNDEFEXP
  #endif // exp
ENDVERBATIM
}
