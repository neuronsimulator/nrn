NEURON {
  ARTIFICIAL_CELL UnitsTest
  RANGE mole, e, faraday, planck, hbar, gasconst
}


UNITS {
  mol = (mole) (1)
  E = (e) (coul)
  FARADAY = (faraday) (coul)
  h = (planck) (joule-sec)
  hb = (hbar) (joule-sec)
  R = (k-mole) (joule/degC)
}

ASSIGNED {
  mole (1)
  e (coul)
  faraday (coul)
  planck (joule-sec)
  hbar (joule-sec)
  gasconst (joule/degC)
}

INITIAL {
  mole = mol
  e = E
  faraday = FARADAY
  planck = h
  hbar = hb
  gasconst = R
}

