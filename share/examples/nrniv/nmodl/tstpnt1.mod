: Demo of POINTER in a POINT_PROCESS

NEURON {
	POINT_PROCESS t1
	POINTER p1, p2
	}

ASSIGNED {
	p1 p2
}

FUNCTION f1() {
	if (nrn_pointing(p1)) {
		f1 = p1
	}else{
		f1 = 0
		printf("p1 does not point anywhere\n")
	}
}
