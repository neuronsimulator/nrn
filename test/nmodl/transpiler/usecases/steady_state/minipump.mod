NEURON {
  SUFFIX minipump
}

PARAMETER {
  volA = 1e9
  volB = 1e9
  volC = 13.0
  kf = 3.0
  kb = 4.0

  run_steady_state = 0.0
}

STATE {
  X
  Y
  Z
}

INITIAL {
  X = 40.0
  Y = 8.0
  Z = 1.0

  if(run_steady_state > 0.0) {
    SOLVE state STEADYSTATE sparse
  }
}

BREAKPOINT {
  SOLVE state METHOD sparse
}

KINETIC state {
  COMPARTMENT volA {X}
  COMPARTMENT volB {Y}
  COMPARTMENT volC {Z}

  ~ X + Y <-> Z (kf, kb)

  CONSERVE Y + Z = 8.0*volB + 1.0*volC
}
