: test of BEFORE and AFTER with no current computation

NEURON {
  SUFFIX ba0
  RANGE bi, i, ai, bb, b, pval, as, bs
  RANGE bit, it, ait, bbt, bt, pvalt, ast, bst
}

ASSIGNED {
  inc
  : Mostly, first letter stands for BEFORE, AFTER.
  : second letter stands for INITIAL, BREAKPOINT, SOLVE
  : last t is for time and if absent, then the value of inc.
  : All but BEFORE INITIAL increments inc
  bi i ai bb b pval as bs
  bit it ait bbt bt pvalt ast bst
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
  printf("ba0 BEFORE INITIAL inc=%g t=%g\n", inc, t)
}

INITIAL {
  inc = inc + 1
  i = inc
  it = t
  printf("ba0 INITIAL inc=%g t=%g\n", inc, t)
}

AFTER INITIAL {
  inc = inc + 1
  ai = inc
  ait = t
  printf("ba0 AFTER INITIAL inc=%g t=%g\n", inc, t)
}

BEFORE BREAKPOINT {
  inc = inc + 1
  bb = inc
  bbt = t
  printf("ba0 BEFORE BREAKPOINT inc=%g t=%g\n", inc, t)
}

BREAKPOINT {
  inc = inc + 1
  b = inc
  bt = t
  printf("ba0 BREAKPOINT inc=%g t=%g\n", inc, t)
  SOLVE p
}

PROCEDURE p() {
  inc = inc + 1
  pval = inc
  pvalt = t
  printf("ba0 PROCEDURE inc=%g t=%g\n", inc, t)
}

AFTER SOLVE {
  inc = inc + 1
  as = inc
  ast = t
  printf("ba0 AFTER SOLVE inc=%g t=%g\n", inc, t)
}

BEFORE STEP {
  inc = inc + 1
  bs = inc
  bst = t
  printf("ba0 BEFORE STEP inc=%g t=%g\n", inc, t)
}
