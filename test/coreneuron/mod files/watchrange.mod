: for testing multiple WATCH statements activated as same time
: high, low, and mid regions watch a random uniform variable.
: The random variable ranges from 0 to 1 and changes at random times in
: the neighborhood of interval tick.

NEURON {
  THREADSAFE
  POINT_PROCESS Bounce
  RANGE r, result, n_high, n_low, n_mid, tick, x, t1
  BBCOREPOINTER ran
}

PARAMETER {
  tick = 0.25 (ms)
  LowThresh = 0.3
  HighThresh = 0.7
}

ASSIGNED {
  x (1)
  t1 (ms)
  r (1)
  n_high (1)
  n_mid (1)
  n_low (1)
  result (1)
  ran
}

DEFINE Low  1
DEFINE Mid  2
DEFINE High  3
DEFINE Clock 4

VERBATIM
#include "nrnran123.h"
ENDVERBATIM

INITIAL {
  VERBATIM
    if (_p_ran) {
      nrnran123_setseq((nrnran123_State*)_p_ran, 0, 0);
    }
  ENDVERBATIM
  n_high = 0
  n_mid = 0
  n_low = 0
  r = uniform()
  t1 = t
  x = 0
  net_send(0, Mid)
  net_send(0, Clock)
}

:AFTER SOLVE {
:  result = t1*100/1(ms) + x
:}

NET_RECEIVE(w) {
  t1 = t
  if (flag == Clock) {
    r = uniform()
    net_send(tick*(uniform() + .5), Clock)
  }
  if (flag == High) {
    x = High
    n_high = n_high + 1
    WATCH (r < LowThresh) Low
    WATCH (r < HighThresh ) Mid
  }else if (flag == Mid) {
    x = Mid
    n_mid = n_mid + 1
    WATCH (r < LowThresh) Low
    WATCH (r > HighThresh) High
  }else if (flag == Low) {
    x = Low
    n_low = n_low + 1
    WATCH (r > HighThresh) High
    WATCH (r > LowThresh) Mid
  }
}

FUNCTION uniform() {
  uniform = 0.5
VERBATIM
  if (_p_ran) {
    _luniform = nrnran123_dblpick((nrnran123_State*)_p_ran);
  }
ENDVERBATIM
}

PROCEDURE noiseFromRandom123() {
VERBATIM
#if !NRNBBCORE
 {
  nrnran123_State** pv = (nrnran123_State**)(&_p_ran);
  if (*pv) {
    nrnran123_deletestream(*pv);
    *pv = (nrnran123_State*)0;
  }
  if (ifarg(1)) {
    *pv = nrnran123_newstream3((uint32_t)*getarg(1), (uint32_t)*getarg(2), (uint32_t)*getarg(3));
  }
 }
#endif
ENDVERBATIM
}

DESTRUCTOR {
VERBATIM
 {
  nrnran123_State** pv = (nrnran123_State**)(&_p_ran);
  if (*pv) {
    nrnran123_deletestream(*pv);
    *pv = nullptr;
  }
 }
ENDVERBATIM
}

VERBATIM
static void bbcore_write(double* z, int* d, int* zz, int* offset, _threadargsproto_) {
  if (d) {
    char which;  
    uint32_t* di = ((uint32_t*)d) + *offset;
    nrnran123_State** pv = (nrnran123_State**)(&_p_ran);
    nrnran123_getids3(*pv, di, di+1, di+2);
    nrnran123_getseq(*pv, di+3, &which);
    di[4] = (int)which;
#if NRNBBCORE
    /* CoreNEURON does not call DESTRUCTOR so ... */
    nrnran123_deletestream(*pv);
    *pv = nullptr;
#endif
  }                     
  *offset += 5;
}

static void bbcore_read(double* z, int* d, int* zz, int* offset, _threadargsproto_) {
  uint32_t* di = ((uint32_t*)d) + *offset;
  nrnran123_State** pv = (nrnran123_State**)(&_p_ran);
#if !NRNBBCORE
  if (*pv) {
    nrnran123_deletestream(*pv);
  }
#endif
  *pv = nrnran123_newstream3(di[0], di[1], di[2]);
  nrnran123_setseq(*pv, di[3], (char)di[4]);
  *offset += 5;
}
ENDVERBATIM
