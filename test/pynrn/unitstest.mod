NEURON {
  ARTIFICIAL_CELL UnitsTest
  RANGE mole, e, faraday, planck, hbar, gasconst, avogadro, k
}


UNITS {
  mol = (mole) (1)
  E = (e) (coul)
  FARADAY = (faraday) (coul)
  h = (planck) (joule-sec)
  hb = (hbar) (joule-sec)
  R = (k-mole) (joule/degC)
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
  avogadro (1)
  k (joule/degC)
}

INITIAL {
  mole = mol
  e = E
  faraday = FARADAY
  planck = h
  hbar = hb
  gasconst = R
  avogadro = avo
  k = boltzmann
}

