: Trivial model to test EXTERNAL statement by providing global variables
: to exter.mod
NEURON {
  POINT_PROCESS Glob
  GLOBAL s, a
}
 
PARAMETER {
  s
  a[3]		
}
