from neuron import h
from neuron.expect_hocerr import expect_err
import neuron.expect_hocerr

neuron.expect_hocerr.quiet = False

h(
    """
begintemplate Foo
public o
objref o[3]
proc init() {local i
  objref o[$1]
  for i = 0, $1-1 {
    o[i] = new Vector(i)
    o[i].fill(i)
  }
}
endtemplate Foo
"""
)

h(
    """
objref foo
foo = new Foo(3)

Foo[0]
"""
)
