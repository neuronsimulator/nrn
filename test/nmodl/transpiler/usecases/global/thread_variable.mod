NEURON {
  SUFFIX shared_global
  NONSPECIFIC_CURRENT il
  RANGE y, z
  GLOBAL g_w, g_arr
  THREADSAFE
}

ASSIGNED {
  y
  z
  g_w
  g_arr[3]
  il
}

INITIAL {
  g_w = 48.0
  g_arr[0] = 10.0 + z
  g_arr[1] = 10.1
  g_arr[2] = 10.2
  y = 10.0
}

BREAKPOINT {
  if(t > 0.33) {
    g_w = g_arr[0] + g_arr[1] + g_arr[2]
  }

  if(t > 0.66) {
    g_w = z
  }
  y = g_w
  il = 0.0000001 * (v - 10.0)
}
