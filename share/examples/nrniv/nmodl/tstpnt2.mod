:Demo of POINTER in Continuous mechanism

NEURON {
	SUFFIX t2
	POINTER p 
	RANGE r
}

ASSIGNED {
	r p
}

FUNCTION f() {
	f = r*100 + p
}
