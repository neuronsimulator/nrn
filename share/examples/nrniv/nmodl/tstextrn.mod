: a test of the new EXTERNAL keyword and its usage.

:Note Ra is built-in to NEURON through the init.c file
:     minf_hh is GLOBAL via the built in hh.mod file.
:	it was declared GLOBAL minf and the SUFFIX was hh

NEURON {
	EXTERNAL clamp_resist, minf_hh
}

ASSIGNED {
	clamp_resist (megohm)
	minf_hh
}

FUNCTION cr() (megohm) {
	cr = clamp_resist
}

FUNCTION minf() {
	minf = minf_hh
}
