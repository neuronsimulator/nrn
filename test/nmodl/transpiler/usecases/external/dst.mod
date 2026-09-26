NEURON {
   SUFFIX dst

   EXTERNAL gbl_src, param_src
}

ASSIGNED {
   gbl_src
   param_src
}

FUNCTION get_gbl() {
   get_gbl = gbl_src
}

FUNCTION get_param() {
   get_param = param_src
}
