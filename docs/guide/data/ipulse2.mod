COMMENT
  ipulse2.mod
  Generates a train of current pulses
  User specifies dur (pulse duration), per (period, i.e. interval between pulse onsets),
  and number of pulses.
  Ensures that period is longer than pulse duration.
  2/6/2002 NTC
ENDCOMMENT


NEURON {
  POINT_PROCESS Ipulse2
  RANGE del, dur, per, num, amp, i
  ELECTRODE_CURRENT i
}

UNITS {
  (nA) = (nanoamp)
}

PARAMETER {
  del (ms)
  dur (ms) <0, 1e9> : duration of ON phase
  per (ms) <0, 1e9> : period of stimuls, i.e. interval between pulse onsets
  num : how many to deliver
  amp (nA) : how big
}

ASSIGNED {
  ival (nA)
  i (nA)
  on
  tally : how many more to deliver
}

INITIAL {
  if (dur >= per) {
    per = dur + 1 (ms)
    printf("per must be longer than dur\n")
UNITSOFF
    printf("per has been increased to %g ms\n", per)
UNITSON
  }
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
      net_send(dur, 1)
    } else {
      : turn it off
      ival = 0
      on = 0
      if (tally > 0) {
        : prepare to turn it on again
        net_send(per - dur, 1)
        tally = tally - 1
      }
    }
  }
}
