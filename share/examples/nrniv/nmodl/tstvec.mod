: adding methods to the Vector class from NMODl

NEURON {
	SUFFIX nothing
}

VERBATIM
static double extra(void* vv) {	/* v is a reserved word in nmodl */
	int i, nx, ny;
	double* x, *y;
	/* how to get the instance data */
	nx = vector_instance_px(vv, &x);
	
	/* how to get a Vector argument */
	ny = vector_arg_px(1, &y);

	for (i=0; i < nx && i < ny; ++i) {
		printf("%d %g %g\n", i, x[i], y[i]);
	}

	return i;
}
ENDVERBATIM

PROCEDURE install_vector_methods() {
VERBATIM
	/* the list of additional methods */
	install_vector_method("extra", extra);
ENDVERBATIM
}

