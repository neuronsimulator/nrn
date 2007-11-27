NEURON {
	POINT_PROCESS gap
	NONSPECIFIC_CURRENT i
	RANGE r, i
	POINTER vgap
}
PARAMETER {
	v (millivolt)
	vgap (millivolt)
	r = 1e10 (megohm)
}
ASSIGNED {
	i (nanoamp)
}
BREAKPOINT {
	i = (v - vgap)/r
}

