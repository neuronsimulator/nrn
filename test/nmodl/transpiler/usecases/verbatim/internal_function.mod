NEURON {
    SUFFIX internal_function
    THREADSAFE
}

FUNCTION f() {
VERBATIM
  return g(_threadargs_);
ENDVERBATIM
}

FUNCTION g() {
  g = 42.0
}
