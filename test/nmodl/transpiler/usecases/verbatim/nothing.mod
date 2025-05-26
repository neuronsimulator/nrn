NEURON { SUFFIX nothing }

VERBATIM
double foo = 42.0;
ENDVERBATIM

FUNCTION get_foo() {
VERBATIM
    return foo;
ENDVERBATIM
}
