: demonstrate the order in which various tasks are done

NEURON {
	SUFFIX order
:	NONSPECIFIC_CURRENT i
	ELECTRODE_CURRENT i
	RANGE index
}

PARAMETER { index = 0 }
ASSIGNED {
	v (millivolt)
	i (milliamp/cm2)
}

INITIAL {
	printf("INITIAL index=%g t=%g v=%g\n", index, t, v)
}

BREAKPOINT {
	printf("BREAKPOINT index=%g t=%g v=%g\n", index, t, v)
	i = 0
}

BEFORE BREAKPOINT {
	printf("BEFORE BREAKPOINT index=%g t=%g v=%g\n", index, t, v)
}

AFTER SOLVE {
	printf("AFTER SOLVE index=%g t=%g v=%g\n", index, t, v)
}

BEFORE INITIAL {
	printf("BEFORE INITIAL index=%g t=%g v=%g\n", index, t, v)
}

AFTER INITIAL {
	printf("AFTER INITIAL index=%g t=%g v=%g\n", index, t, v)
}


