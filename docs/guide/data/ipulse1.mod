COMMENT
  ipulse1.mod
  Generates a train of current pulses
  User specifies duration of pulse, interpulse interval (ton and toff),
  and number of pulses.
  1/24/2002 NTC
ENDCOMMENT


NEURON {
  POINT_PROCESS Ipulse1
  RANGE del, ton, toff, num, amp, i
  ELECTRODE_CURRENT i
}

UNITS {
  (nA) = (nanoamp)
}

PARAMETER {
  del (ms)
  ton (ms) <0, 1e9> : duration of ON phase
  toff (ms) <0, 1e9> : duration of OFF phase
  num : how many to deliver
  amp (nA) : how big
}

ASSIGNED {
  ival (nA)
  i (nA)
  on
  tally  : how many more to deliver
}

INITIAL {
  on = 0
  i = 0
  ival = 0
  tally = num
  if (tally > 0) {
    net_send(del, 1)
    tally = tally - 1
  }
}

BREAKPOINT {
: printf("%g\n", t)
  i = ival
}

NET_RECEIVE (w) {
  : ignore any but self-events with flag == 1
  if (flag == 1) {
    if (on == 0) {
      : turn it on
      ival = amp
      on = 1
      : prepare to turn it off
      net_send(ton, 1)
    } else {
      : turn it off
      ival = 0
      on = 0
      if (tally > 0) {
        : prepare to turn it on again
        net_send(toff, 1)
        tally = tally - 1
      }
    }
  }
}
