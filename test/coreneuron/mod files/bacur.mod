: test of BEFORE and AFTER in a current density model

NEURON {
  SUFFIX ba1
  NONSPECIFIC_CURRENT icur
  RANGE bi, i, ai, bb, b, pval, as, bs
  RANGE bit, it, ait, bbt, bt, pvalt, ast, bst
}

ASSIGNED {
  icur
  inc
  : Mostly, first letter stands for BEFORE, AFTER.
  : second letter stands for INITIAL, BREAKPOINT, SOLVE
  : last t is for time and if absent, then the value of inc.
  : All but BEFORE INITIAL increments inc
  bi i ai bb b pval as bs
  bit it ait bbt bt pvalt ast bst
}

STATE {
  s
}

PROCEDURE init() {
  LOCAL m
  m = -1
  inc=m
  bi=m i=m ai=m bb=m b=m pval=m as=m bs=m  
  bit=m it=m ait=m bbt=m bt=m pvalt=m ast=m bst=m
}

BEFORE INITIAL {
  init()
  inc = 0
  bi = inc
  bit = t
  printf("ba1 BEFORE INITIAL inc=%g v=%g t=%g\n", inc, v, t)
}

INITIAL {
  inc = inc + 1
  i = inc
  it = t
  printf("ba1 INITIAL inc=%g v=%g t=%g\n", inc, v, t)
}

AFTER INITIAL {
  inc = inc + 1
  ai = inc
  ait = t
  printf("ba1 AFTER INITIAL inc=%g v=%g t=%g\n", inc, v, t)
}

BEFORE BREAKPOINT {
  inc = inc + 1
  bb = inc
  bbt = t
  printf("ba1 BEFORE BREAKPOINT inc=%g v=%g t=%g\n", inc, v, t)
}

BREAKPOINT {
  inc = inc + 1
  b = inc
  bt = t
  printf("ba1 BREAKPOINT inc=%g v=%g t=%g\n", inc, v, t)
  SOLVE deriv METHOD cnexp
  icur = 0
}

DERIVATIVE deriv {
  inc = inc + 1
  pval = inc
  pvalt = t  
  s' = 10
  printf("ba1 DERIVATIVE inc=%g v=%g t=%g\n", inc, v, t)
}

AFTER SOLVE {
  inc = inc + 1
  as = inc
  ast = t
  printf("ba1 AFTER SOLVE inc=%g v=%g t=%g\n", inc, v, t)
}

BEFORE STEP {
  inc = inc + 1
  bs = inc
  bst = t
  printf("ba1 BEFORE STEP inc=%g v=%g t=%g\n", inc, v, t)
}
