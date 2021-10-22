:  Vector stream of events

COMMENT
A VecStim is an artificial spiking cell that generates
events at times that are specified in a Vector.

HOC Example:

// assumes spt is a Vector whose elements are all > 0
// and are sorted in monotonically increasing order
objref vs
vs = new VecStim()
vs.play(spt)
// now launch a simulation, and vs will produce spike events
// at the times contained in spt

Python Example:

from neuron import h
spt = h.Vector(10).indgen(1, 0.2)
vs = h.VecStim()
vs.play(spt)

def pr():
  print (h.t)

nc = h.NetCon(vs, None)
nc.record(pr)

cvode = h.CVode()
h.finitialize()
cvode.solve(20)

ENDCOMMENT

NEURON {
	THREADSAFE
	ARTIFICIAL_CELL VecStim
	BBCOREPOINTER ptr
}

ASSIGNED {
	index
	etime (ms)
	ptr
}


INITIAL {
	index = 0
	element()
	if (index > 0) {
		net_send(etime - t, 1)
	}
}

NET_RECEIVE (w) {
	if (flag == 1) {
		net_event(t)
		element()
		if (index > 0) {
			net_send(etime - t, 1)
		}
	}
}

DESTRUCTOR {
VERBATIM
#if !NRNBBCORE
	void* vv = (void*)(_p_ptr);  
        if (vv) {
		hoc_obj_unref(*vector_pobj(vv));
	}
#endif
ENDVERBATIM
}

PROCEDURE element() {
VERBATIM	
  { void* vv; int i, size; double* px;
	i = (int)index;
	if (i >= 0) {
		vv = (void*)(_p_ptr);
		if (vv) {
			size = vector_capacity(vv);
			px = vector_vec(vv);
			if (i < size) {
				etime = px[i];
				index += 1.;
			}else{
				index = -1.;
			}
		}else{
			index = -1.;
		}
	}
  }
ENDVERBATIM
}

PROCEDURE play() {
VERBATIM
#if !NRNBBCORE
  {
	void** pv;
	void* ptmp = NULL;
	if (ifarg(1)) {
		ptmp = vector_arg(1);
		hoc_obj_ref(*vector_pobj(ptmp));
	}
	pv = (void**)(&_p_ptr);
	if (*pv) {
		hoc_obj_unref(*vector_pobj(*pv));
	}
	*pv = ptmp;
  }
#endif
ENDVERBATIM
}

VERBATIM
static void bbcore_write(double* xarray, int* iarray, int* xoffset, int* ioffset, _threadargsproto_) {
  int i, dsize, *ia;
  double *xa, *dv;
  dsize = 0;
  if (_p_ptr) {
    dsize = vector_capacity(_p_ptr);
  }
  if (iarray) {
    void* vec = _p_ptr;
    ia = iarray + *ioffset;
    xa = xarray + *xoffset;
    ia[0] = dsize;
    if (dsize) {
      dv = vector_vec(vec);
      for (i = 0; i < dsize; ++i) {
         xa[i] = dv[i];
      }
    }
  }
  *ioffset += 1;
  *xoffset += dsize;
}

static void bbcore_read(double* xarray, int* iarray, int* xoffset, int* ioffset, _threadargsproto_) {
  int dsize, i, *ia;
  double *xa, *dv;
  xa = xarray + *xoffset;
  ia = iarray + *ioffset;
  dsize = ia[0];
  if (!_p_ptr) {
    _p_ptr = vector_new1(dsize);
  }
  assert(dsize == vector_capacity(_p_ptr));
  dv = vector_vec(_p_ptr);
  for (i = 0; i < dsize; ++i) {
    dv[i] = xa[i];
  }
  *xoffset += dsize;
  *ioffset += 1;
}

ENDVERBATIM
