: a test of the various Variable Types
: (RANGE, GLOBAL, POINTER, BBCOREPOINTER, EXTERNAL, PARAMETER, ASSIGNED)

NEURON {
	SUFFIX testvars
	THREADSAFE
	GLOBAL global_var
	RANGE range_var, parameter_var
	POINTER p1
	BBCOREPOINTER my_data : changed from POINTER
	EXTERNAL clamp_resist : 
}

PARAMETER {
	parameter_var
	global_var
}

ASSIGNED {
	range_var
	p1
	my_data
	clamp_resist
}

INITIAL {
	range_var = 42
}

FUNCTION cr() (megohm) {
	cr = clamp_resist
}

FUNCTION f1() {
	if (nrn_pointing(p1)) {
		f1 = p1
	}else{
		f1 = 0
		printf("p1 does not point anywhere\n")
	}
}

: Do something interesting with my_data ...
VERBATIM
static void bbcore_write(double* x, int* d, int* x_offset, int* d_offset, _threadargsproto_) {
  if (x) {
    double* x_i = x + *x_offset;
    x_i[0] = ((double*)_p_my_data)[0];
    x_i[1] = ((double*)_p_my_data)[1];
  }
  *x_offset += 2; // reserve 2 doubles on serialization buffer x
}

static void bbcore_read(double* x, int* d, int* x_offset, int* d_offset, _threadargsproto_) {
  assert(!_p_my_data);
  double* x_i = x + *x_offset;
  // my_data needs to be allocated somehow
  _p_my_data = (double*)malloc(sizeof(double)*2);
  ((double*)_p_my_data)[0] = x_i[0];
  ((double*)_p_my_data)[1] = x_i[1];
  *x_offset += 2;
}
ENDVERBATIM
