: a test of the new EXTERNAL keyword and its usage.

:     minf_hh is GLOBAL via the built in hh.mod file.
:	it was declared GLOBAL minf and the SUFFIX was hh
:       edit: unfortunately since hh.mod is THREADSAFE and minf is assigned
:           within the mod file, minf become a thread variable and no longer
:           is available. For this test we have replaced by the eps GLOBAL
:           in intfire4.mod

NEURON {
	EXTERNAL clamp_resist, eps_IntFire4
}

ASSIGNED {
	clamp_resist (megohm)
	eps_IntFire4
}

FUNCTION cr() (megohm) {
	cr = clamp_resist
}

FUNCTION eps() {
	eps = eps_IntFire4
}
