NEURON {
  POINT_PROCESS hhwatch
  NONSPECIFIC_CURRENT i
  GLOBAL ena, ek, erev, gna, gk, gpas
  RANGE e, g
}

UNITS {
  (mV) = (millivolt)
  (nA) = (nanoamp)
  (umho) = (micromho)
}

PARAMETER {
  ena = 50 (mV)
  ek = -80 (mV)
  erev = -65 (mV)
  gna = 0.1 (umho)
  gk = 0.03 (umho)
  gpas = 0.0001 (umho)
}

ASSIGNED {
  v (mV)
  i (nA)
  e (mV)
  g (umho)
}

DEFINE init 1
DEFINE rise 2
DEFINE fall 3
DEFINE off 4

INITIAL {
  g = gpas
  e = erev
  net_send(0, init)
}

BREAKPOINT {
  i = g*(v - e)
}

NET_RECEIVE(w) {
  if (flag == init) {
    WATCH (v > -55) rise
  }else if (flag == rise) {
    g = gna
    e = ena
    WATCH (v > 10) fall
  }else if (flag == fall) {
    g = gk
    e = ek
    WATCH (v < -70) off
  }else if (flag == off) {
    g = gpas
    e = erev
    WATCH (v > -55) rise
  }
}
