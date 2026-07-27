COMMENT
  Density mechanism with PROCEDURE dforce() for IDA IC tests.
  Mirrors the idea of models/dcmdt dcdt.mod analytic dcmdt(t):
  at IDA reinit, NEURON calls dforce() with t set to the IC time so
  assigned rates reflect the classical right limit at t+.
ENDCOMMENT

NEURON {
  SUFFIX moddforce
  THREADSAFE
  RANGE rate, amp, omega, t0
}

PARAMETER {
  amp = 1
  omega = 1  (/ms)
  t0 = 0 (ms)
}

ASSIGNED {
  rate
  v (mV)
}

INITIAL {
  rate = 0
}

COMMENT
  Called automatically at each IDA finitialize/reinit when dae_init_mode is used
  (and whenever Daspk::init runs). Sets rate = amp*omega*cos(omega*(t-t0)).
ENDCOMMENT
PROCEDURE dforce() {
  rate = amp * omega * cos(omega * (t - t0))
}
