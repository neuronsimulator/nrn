NEURON {
	POINT_PROCESS APCount
	RANGE n, thresh, time, firing
	THREADSAFE : if APCount.record uses distinct instances of Vector
}

UNITS {
	(mV) = (millivolt)
}

PARAMETER {
	n
	thresh = -20 (mV)
	time (ms)
}

ASSIGNED {
	firing
	space
}

INITIAL {
	n = 0
	firing = 0
VERBATIM
	if (auto* vv = *reinterpret_cast<IvocVect**>(&space)) {
		vector_resize(vv, 0);
	}
ENDVERBATIM
	check()
}

BREAKPOINT {
	SOLVE check METHOD after_cvode
}

PROCEDURE check() {
VERBATIM
	if (v >= thresh && !firing) {
		int size; double* px;
		firing = 1;
		time = t;
		n += 1.;
		if (auto* vv = *reinterpret_cast<IvocVect**>(&space)) {
			size = (int)n;
			vector_resize(vv, size);
			px = vector_vec(vv);
			px[size-1] = time;
		}
	}
	if (firing && v < thresh && t > time) {
		firing = 0;
	}
ENDVERBATIM
}

PROCEDURE record() {
VERBATIM
	auto* vv = reinterpret_cast<IvocVect**>(&space);
	*vv = nullptr;
	if (ifarg(1)) {
		*vv = vector_arg(1);
	}
ENDVERBATIM
}
