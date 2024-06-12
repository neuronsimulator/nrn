: For testing EXTERNAL. Mirror GLOBAL from glob.mod
NEURON {
  THREADSAFE
  POINT_PROCESS Exter
  EXTERNAL s_Glob, a_Glob
  RANGE std
}

PARAMETER {
  s_Glob
  a_Glob[1] : size does not matter
  std
}

PROCEDURE set_s(x) {
  s_Glob = x
}

PROCEDURE set_a(i, x) {
  a_Glob[i] = x
}

FUNCTION get_s() {
  get_s = s_Glob
}

FUNCTION get_a(i) {
  get_a = a_Glob[i]
}

INITIAL {
  std = 0
  s_Glob = 0
  FROM i = 0 TO 2 {
    a_Glob[i] = 0
  }
}

BEFORE STEP {
  std = std + 1
  s_Glob = s_Glob + 1
  FROM i = 0 TO 2 {
    a_Glob[i] = a_Glob[i] + .1^(i+1)
  }
}
