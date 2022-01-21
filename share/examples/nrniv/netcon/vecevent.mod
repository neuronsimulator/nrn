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
	auto* vv = reinterpret_cast<IvocVect*>(_p_ptr);
	if (vv) {
		hoc_obj_unref(*vector_pobj(vv));
	}
#endif
ENDVERBATIM
}

PROCEDURE element() {
VERBATIM	
  { int i, size; double* px;
	i = (int)index;
	if (i >= 0) {
		auto* vv = reinterpret_cast<IvocVect*>(_p_ptr);
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
	IvocVect* ptmp{};
	if (ifarg(1)) {
		ptmp = vector_arg(1);
		hoc_obj_ref(*vector_pobj(ptmp));
	}
	auto* pv = reinterpret_cast<IvocVect**>(&_p_ptr);
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
	auto* vec = reinterpret_cast<IvocVect*>(_p_ptr);
  if (vec) {
    dsize = vector_capacity(vec);
  }
  if (iarray) {
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
	auto* vec = reinterpret_cast<IvocVect*>(_p_ptr);
  if (!vec) {
    vec = vector_new1(dsize);
  }
  assert(dsize == vector_capacity(vec));
  dv = vector_vec(vec);
	_p_ptr = reinterpret_cast<double*>(vec);
  for (i = 0; i < dsize; ++i) {
    dv[i] = xa[i];
  }
  *xoffset += dsize;
  *ioffset += 1;
}

ENDVERBATIM
