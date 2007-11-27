:example of faster fit function for use with praxis.

NEURON {
	SUFFIX fit1
	GLOBAL data
}	

ASSIGNED {
	data[30]
}


FUNCTION err(n) {
	LOCAL d, x, a, k
VERBATIM
	{
		double* pp, *hoc_pgetarg();
		pp = hoc_pgetarg(2);
		_la = pp[0];
		_lk = pp[1];
	}
ENDVERBATIM
	err = 0
	FROM i=0 TO 29 {
		x = i/10
		d = data[i] - fun(a, k, x)
		err= err + d*d
	}
}

FUNCTION fun(a, k, x) {
	fun = a * exp(-k*x)
}
