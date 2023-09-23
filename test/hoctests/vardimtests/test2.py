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

h("""double array[4]""")
expect_err("h.array = 5")
h("""objref oarray[4][4]""")
# cannot reach line we want because of earlier array check
expect_err("h.oarray[2] = []")


# test assignment/evaluation of mod file array variable from python
s = h.Section()
s.nseg = 3
s.insert("atst")
for seg in s:
    ar = seg.atst.arrayrng
    assert len(ar) == 4
    for i in range(len(ar)):
        ar[i] = i + seg.x
        seg.arrayrng_atst[i] = ar[i]
for seg in s:
    ar = seg.atst.arrayrng
    for i in range(len(ar)):
        assert ar[i] == i + seg.x
        assert seg.arrayrng_atst[i] == i + seg.x

ar = s.arrayrng_atst
assert len(ar) == 4
for i in range(len(ar)):
    ar[i] = i
for i in range(len(ar)):
    assert ar[i] == float(i)

expect_err("print(ar[10])")
expect_err("ar[10] = 1")
