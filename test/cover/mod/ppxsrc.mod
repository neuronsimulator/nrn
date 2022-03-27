: For testing source POINT_PROCESS with "x" threshold variable
: Such a POINT_PROCESS is discouraged in favor of NET_RECEIVE with WATCH

NEURON {
  POINT_PROCESS PPxSrc
  RANGE x
}

ASSIGNED {
  v (millivolt)
  x (millivolt)
}

INITIAL {
  x = v
}

BREAKPOINT {
  x = v
}

