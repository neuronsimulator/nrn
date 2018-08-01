
UNITS {
  (mA) = (milliamp)
  (mV) = (millivolt)

}

PARAMETER {
  v (mV)
  gca=.003 (mho/cm2)
  myeca=-64 (mV)
}


NEURON {
  SUFFIX caleak
  USEION ca WRITE ica
  RANGE gca, ica
}


ASSIGNED {
  ica (mA/cm2)
}

INITIAL {
}

BREAKPOINT {
  ica = gca*(v-myeca)
}



