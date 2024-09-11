NEURON {
    POINT_PROCESS ctor
    GLOBAL ctor_calls, dtor_calls
}

ASSIGNED {
  ctor_calls
  dtor_calls
}

CONSTRUCTOR {
  ctor_calls = ctor_calls + 1
}

DESTRUCTOR {
  dtor_calls = dtor_calls + 1
}
