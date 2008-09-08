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

VERBATIM
extern void vector_resize();
extern double* vector_vec();
extern void* vector_arg();
ENDVERBATIM

INITIAL {
	n = 0
	firing = 0
VERBATIM
	{ void* vv;
		vv = *((void**)(&space));
		if (vv) {
			vector_resize(vv, 0);
		}
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
		int size; double* px; void* vv;
		firing = 1;
		time = t;
		n += 1.;
		vv = *((void**)(&space));
		if (vv) {
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
	extern void* vector_arg();
	void** vv;
	vv = (void**)(&space);
	*vv = (void*)0;
	if (ifarg(1)) {
		*vv = vector_arg(1);
	}
ENDVERBATIM
}
