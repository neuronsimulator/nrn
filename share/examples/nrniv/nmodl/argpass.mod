NEURON{ SUFFIX nothing}

: in hoc, call with testpass("string", number)

PROCEDURE testpass() {
VERBATIM
	printf("%s %g\n", gargstr(1), *getarg(2));
ENDVERBATIM
}
