COMMENT
  ipulse3.mod
  Generates a current pulse when it receives an input event.
  User specifies dur (pulse duration).
  Ignores events that arrive during an ongoing pulse.
  2/3/2012 NTC
ENDCOMMENT


NEURON {
  POINT_PROCESS Ipulse3
  RANGE dur, amp, i
  ELECTRODE_CURRENT i
}

UNITS {
  (nA) = (nanoamp)
}

PARAMETER {
  dur (ms) <0, 1e9> : duration of ON phase
  amp (nA) : how big
}

ASSIGNED {
  ival (nA)
  i (nA)
  on
}

INITIAL {
  on = 0
  i = 0
  ival = 0
}

BREAKPOINT {
  i = ival
}

NET_RECEIVE (w) {
  if (flag == 0) { : not a self event
    if (on == 0) {
      : turn it on
      ival = amp
      on = 1
      : prepare to turn it off
      net_send(dur, 1)
    }
  } else { : a self event
    : turn it off
    ival = 0
    on = 0
  }
}
