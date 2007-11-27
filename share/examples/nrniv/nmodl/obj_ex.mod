NEURON {
	POINT_PROCESS ObjectExample
	RANGE a, x
}

PARAMETER {a}
ASSIGNED {x[3]}

FUNCTION aval() {
	aval = a
}

PROCEDURE prx() {
	FROM i=0 TO 2 {
		printf ("x[%g] = %g\n", i, x[i])
	}
}

COMMENT
This model demonstrates how to add a new class as an alternative to the
begintemplate interpreted method. Unfortunately the instantiation of
an object like this must take place within a section but that is not
too odious since one can always define a utility section that is
not connected to any other section.
ENDCOMMENT
