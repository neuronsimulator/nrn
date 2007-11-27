NEURON{
	SUFFIX nothing
	POINTER y
}

ASSIGNED {y[1]}

PROCEDURE set(i, x) {
	y[i] = x
}

FUNCTION get(i) {
	get = y[i]
}

