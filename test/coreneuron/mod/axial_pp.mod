NEURON {
    POINT_PROCESS AxialPP
    RANGE iaSum
}

INCLUDE "axial.inc"


: Just for testing coreneuron file mode checkpoint by integrating
: abs(ia) and using that to generate spikes (which are automatically
: output).

ASSIGNED {
  iaSum (nA)
}

INITIAL {
  iaSum = 0
  net_send(0, 1)
}

BEFORE STEP {
  if (ri > 0) {
    pim = pim - ia : child contributions
    iaSum = iaSum + fabs(ia)
  }
}

NET_RECEIVE(w) {
  if (flag == 1) {
    WATCH (iaSum > .5) 2
  } else if (flag == 2) {
    iaSum = 0
    net_event(t)
  }
}
