NEURON {
  POINT_PROCESS UnitsTest
  RANGE mole, e, faraday, planck, hbar, gasconst, gasconst_exact, avogadro, k
  RANGE erev, ghk
  USEION na READ ena WRITE ina
}


UNITS {
  (mM) = (milli/liter)
  (mV) = (millivolt)
  mol = (mole) (1)
  E = (e) (coul)
  FARADAY = (faraday) (coul)
  h = (planck) (joule-sec)
  hb = (hbar) (joule-sec)
  R = (k-mole) (joule/degC)
  Rexact = 8.313424 (joule/degC)
  boltzmann = (k) (joule/degC)

  (avogadro) = (mole)
  avo  = (avogadro) (1)
}

ASSIGNED {
  mole (1)
  e (coul)
  faraday (coul)
  planck (joule-sec)
  hbar (joule-sec)
  gasconst (joule/degC)
  gasconst_exact (joule/degC)
  avogadro (1)
  k (joule/degC)
  erev (mV)
  ghk (millicoul/cm3)
  ena (mV)
  ina (nanoamp)
}

INITIAL {
  mole = mol
  e = E
  faraday = FARADAY
  planck = h
  hbar = hb
  gasconst = R
  gasconst_exact = Rexact
  avogadro = avo
  k = boltzmann
  erev = ena
  ghk = nrn_ghk(-50(mV), .001(mM), 10(mM), 2)
}

BREAKPOINT {
  ina = 0
}
