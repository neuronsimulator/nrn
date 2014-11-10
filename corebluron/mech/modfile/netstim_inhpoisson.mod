COMMENT
/**
 * @file netstim_inhpoisson.mod
 * @brief 
 * @author ebmuller
 * @date 2011-03-16
 * @remark Copyright © BBP/EPFL 2005-2011; All rights reserved. Do not distribute without further notice.
 */
ENDCOMMENT

: Inhibitory poisson generator by the thinning method.
: See:  
:   Muller, Buesing, Schemmel, Meier (2007). "Spike-Frequency Adapting
:   Neural Ensembles: Beyond Mean Adaptation and Renewal Theories",
:   Neural Computation 19:11, 2958-3010.
:   doi:10.1162/neco.2007.19.11.2958
:
: Based on vecstim.mod and netstim2.mod shipped with PyNN
: Author: Eilif Muller, 2011

NEURON {
THREADSAFE
  ARTIFICIAL_CELL InhPoissonStim
  RANGE rmax
  RANGE duration
  BBCOREPOINTER uniform_rng, exp_rng, vecRate, vecTbins
  :THREADSAFE : only true if every instance has its own distinct Random
}


VERBATIM
extern int ifarg(int iarg);
extern double* vector_vec(void* vv);
extern void* vector_new1(int _i);
extern int vector_capacity(void* vv);
extern void* vector_arg(int iarg);
double nrn_random_pick(void* r);
void* nrn_random_arg(int argpos);
ENDVERBATIM


PARAMETER {
  interval_min = 1.0  : average spike interval of surrogate Poisson process
  duration	= 1e6 (ms) <0,1e9>   : duration of firing (msec)
}

VERBATIM
#include "nrnran123.h"
ENDVERBATIM

ASSIGNED {
   vecRate
   vecTbins
   index
   curRate
   start (ms)
   event (ms)
   uniform_rng
   exp_rng
   rmax

}

INITIAL {
   index = 0.

   : determine start of spiking.
   VERBATIM
   if (_p_uniform_rng && _p_exp_rng)
   {
     nrnran123_setseq((nrnran123_State*)_p_uniform_rng, 0, 0);
     nrnran123_setseq((nrnran123_State*)_p_exp_rng, 0, 0);
   }

   void *vvTbins = _p_vecTbins;
   double* px;

   if (vvTbins && vector_capacity(vvTbins)>=1) {
     px = vector_vec(vvTbins);
     start = px[0];
     if (start < 0.0) start=0.0;
   }
   else start = 0.0;

   /* first event is at the start 
   TODO: This should draw from a more appropriate dist
   that has the surrogate process starting a t=-inf
   */
   event = start;

   /* set curRate */
   void *vvRate = _p_vecRate;
   px = vector_vec(vvRate);

   /* set rmax */
   rmax = 0.0;
   int i;
   for (i=0;i<vector_capacity(vvRate);i++) {
      if (px[i]>rmax) rmax = px[i];
   }

   if (vvRate && vector_capacity(vvRate)>0) {
     curRate = px[0];
   }
   else {
      curRate = 1.0;
      rmax = 1.0;
   }

   ENDVERBATIM

   update_time()
   erand() : for some reason, the first erand() call seems
           : to give implausibly large values, so we discard it
   generate_next_event()
   : stop even producing surrogate events if we are past duration
   if (t+event < start+duration) {
     net_send(event, 0)
   }

 
}

: This procedure queues the next surrogate event in the 
: poisson process (rate=ramx) to be thinned.
PROCEDURE generate_next_event() {

	event = 1000.0/rmax*erand()

	: but not earlier than 0
	if (event < 0) {
		event = 0
	}

}

: 1st arg: exp_rng
: 2nd arg: uniform_rng
PROCEDURE setRNGs() {
VERBATIM
{
#if !NRNBBCORE
        nrnran123_State** pv = (nrnran123_State**)(&_p_exp_rng);
        if (*pv) {
                nrnran123_deletestream(*pv);
                *pv = (nrnran123_State*)0;
        } 
        if (ifarg(2)) {
                *pv = nrnran123_newstream((uint32_t)*getarg(1), (uint32_t)*getarg(2));
        }

        pv = (nrnran123_State**)(&_p_uniform_rng);
        if (*pv) {
                nrnran123_deletestream(*pv);
                *pv = (nrnran123_State*)0;
        } 
        if (ifarg(4)) {
                *pv = nrnran123_newstream((uint32_t)*getarg(3), (uint32_t)*getarg(4));
        }
#endif
}
ENDVERBATIM
}


FUNCTION urand() {
VERBATIM
	if (_p_uniform_rng) {
		/*
		:Supports separate independent but reproducible streams for
		: each instance. However, the corresponding hoc Random
		: distribution MUST be set to Random.uniform(0,1)
		*/
		_lurand = nrnran123_dblpick((nrnran123_State*)_p_uniform_rng);
	}else{
          _lurand = 0.0;
  	  hoc_execerror("multithread random in NetStim"," only via hoc Random");
	}
ENDVERBATIM
}

FUNCTION erand() {
VERBATIM
	if (_p_exp_rng) {
		/*
		:Supports separate independent but reproducible streams for
		: each instance. However, the corresponding hoc Random
		: distribution MUST be set to Random.negexp(1)
		*/
		_lerand = nrnran123_negexp((nrnran123_State*)_p_exp_rng);
	}else{
          _lerand = 0.0;
  	  hoc_execerror("multithread random in NetStim"," only via hoc Random");
	}
ENDVERBATIM
}

VERBATIM
static void bbcore_write(double* dArray, int* iArray, int* doffset, int* ioffset, _threadargsproto_) {
        uint32_t dsize = 0;
        if (_p_vecRate)
        {
          dsize = (uint32_t)vector_capacity(_p_vecRate);
        }
        if (iArray) {
                uint32_t* ia = ((uint32_t*)iArray) + *ioffset;
                nrnran123_State** pv = (nrnran123_State**)(&_p_exp_rng);
                nrnran123_getids(*pv, ia, ia+1);

                ia = ia + 2;
                pv = (nrnran123_State**)(&_p_uniform_rng);
                nrnran123_getids(*pv, ia, ia+1);

                ia = ia + 2;
                void* vec = _p_vecRate;
                ia[0] = dsize;

                double *da = dArray + *doffset;
                double *dv;
                if(dsize)
                {
                  dv = vector_vec(vec);
                }
                int iInt;
                for (iInt = 0; iInt < dsize; ++iInt)
                {
                  da[iInt] = dv[iInt];
                }

                vec = _p_vecTbins;
                da = dArray + *doffset + dsize;
                if(dsize)
                {
                  dv = vector_vec(vec);
                }
                for (iInt = 0; iInt < dsize; ++iInt)
                {
                  da[iInt] = dv[iInt];
                }
        }
        *ioffset += 5;
        *doffset += 2*dsize;

}

static void bbcore_read(double* dArray, int* iArray, int* doffset, int* ioffset, _threadargsproto_) {
        assert(!_p_exp_rng);
        assert(!_p_uniform_rng);
        assert(!_p_vecRate);
        assert(!_p_vecTbins);
        uint32_t* ia = ((uint32_t*)iArray) + *ioffset;
        nrnran123_State** pv;
        if (ia[0] != 0 || ia[1] != 0)
        {
          pv = (nrnran123_State**)(&_p_exp_rng);
          *pv = nrnran123_newstream(ia[0], ia[1]);
        }

        ia = ia + 2;
        if (ia[0] != 0 || ia[1] != 0)
        {
          pv = (nrnran123_State**)(&_p_uniform_rng);
          *pv = nrnran123_newstream(ia[0], ia[1]);
        }

        ia = ia + 2;
        int dsize = ia[0];
        *ioffset += 5;

        double *da = dArray + *doffset;
        _p_vecRate = vector_new1(dsize);  /* works for dsize=0 */
        double *dv = vector_vec(_p_vecRate);
        int iInt;
        for (iInt = 0; iInt < dsize; ++iInt)
        {
          dv[iInt] = da[iInt];
        }
        *doffset += dsize;

        da = dArray + *doffset;
        _p_vecTbins = vector_new1(dsize);
        dv = vector_vec(_p_vecTbins);
        for (iInt = 0; iInt < dsize; ++iInt)
        {     
          dv[iInt] = da[iInt];
        }
        *doffset += dsize; 
}
ENDVERBATIM






PROCEDURE setTbins() {
VERBATIM
#if !NRNBBCORE
  void** vv;
  vv = &_p_vecTbins;
  *vv = (void*)0;

  if (ifarg(1)) {
    *vv = vector_arg(1);
  }
#endif
ENDVERBATIM
}


PROCEDURE setRate() {
VERBATIM
#if !NRNBBCORE
  void** vv;
  vv = &_p_vecRate;
  *vv = (void*)0;

  if (ifarg(1)) {
    *vv = vector_arg(1);

    int size = vector_capacity(*vv);
    int i;
    double max=0.0;
    double* px = vector_vec(*vv);
    for (i=0;i<size;i++) {
    	if (px[i]>max) max = px[i];
    }
    rmax = max;
  }
#endif
ENDVERBATIM
}

PROCEDURE update_time() {
VERBATIM
  void* vv; int i, i_prev, size; double* px;
  i = (int)index;
  i_prev = i;

  if (i >= 0) { // are we disabled?
    vv = _p_vecTbins;
    if (vv) {
      size = vector_capacity(vv);
      px = vector_vec(vv);
      /* advance to current tbins without exceeding array bounds */
      while ((i+1 < size) && (t>=px[i+1])) {
	index += 1.;
	i += 1;
      }
      /* did the index change? */
      if (i!=i_prev) {
        /* advance curRate to next vecRate if possible */
        void *vvRate = _p_vecRate;
        if (vvRate && vector_capacity(vvRate)>i) {
          px = vector_vec(vvRate);
          curRate = px[i];
        }
        else curRate = 1.0;
      }

      /* have we hit last bin? ... disable time advancing leaving curRate as it is*/
      if (i==size)
        index = -1.;

    } else { /* no vecTbins, use some defaults */
      rmax = 1.0;
      curRate = 1.0;
      index = -1.; /* no vecTbins ... disable time advancing & Poisson unit rate. */
    }
  }

ENDVERBATIM
}


NET_RECEIVE (w) {
	if (flag == 0) { : external event
	  update_time()
	  generate_next_event()
          : stop even producing surrogate events if we are past duration
          if (t+event < start+duration) {
            net_send(event, 0)
          }

          : trigger event
VERBATIM	    
          double u = (double)urand(_threadargs_);
	  /*printf("InhPoisson: spike time at time %g urand=%g curRate=%g, rmax=%g, curRate/rmax=%g \n",t, u, curRate, rmax, curRate/rmax);*/
	  if (u<curRate/rmax) {
ENDVERBATIM
	    :printf("InhPoisson: spike time at time %g\n",t)
            net_event(t)
VERBATIM	    
          }
ENDVERBATIM


	}	  
}

