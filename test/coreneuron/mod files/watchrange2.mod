: for testing multiple WATCH statements activated as same time
: high, low, and mid regions watch a random uniform variable.
: The random variable ranges from 0 to 1 and changes at random times in
: the neighborhood of interval tick.

NEURON {
  THREADSAFE
  POINT_PROCESS Bounce2
  RANGE r, result, n_high, n_low, n_mid, tick, x, t1
  RANDOM ran
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
}

DEFINE Low  1
DEFINE Mid  2
DEFINE High  3
DEFINE Clock 4

INITIAL {
  random_setseq(ran, 0)
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
 uniform = random_uniform(ran)
}
