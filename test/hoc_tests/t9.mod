: test of BEFORE and AFTER

NEURON {
	SUFFIX t9
	RANGE b, x, y, z
}

ASSIGNED {b x y z}

INITIAL {
	if (x != 20) { b = 0 }
	x = x + 1
	:printf("in %g %g %g %g %g\n", b, x, y, z, t)
}

BEFORE INITIAL {
	b = 1
	x = 20
	y = 0
	z = 0
	:printf("bi %g %g  %g %g %g\n", b, x, y, z, t)
}

AFTER INITIAL {
	if (x != 21) { b = 0 }
	x = x + 1
	:printf("ai %g %g %g %g %g\n", b, x, y, z, t)
}

BEFORE BREAKPOINT {
	if (y == 0) { : end of finitialize or beginning of first step
		if (x != 22) { b = 0 }
	}
	x = 24
	y = y + 1
	:printf("bb %g %g %g %g %g\n", b, x, y, z, t)
}

AFTER SOLVE {
	if (x != 24) { b = 0 }
	x = 25
	z = z + 1
	:printf("as %g %g %g %g %g\n", b, x, y, z, t)
}
