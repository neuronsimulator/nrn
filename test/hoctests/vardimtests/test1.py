from neuron import h
from neuron.expect_hocerr import expect_err
import neuron.expect_hocerr

neuron.expect_hocerr.quiet = False

h("objref po")
h("po = new List()")
print(h.po)
h.po = {}
print(h.po)


def mkfoo():
    h(
        r"""obfunc foo() {
  po = new Vector()
  return po
  }
  """
    )


mkfoo()
print(h.foo())

h("objref po[3]")
h.po[1] = [1]
print(h.po, h.po[1])
h("objref po[2][2]")
h.po[1][1] = [1, 2, 3]
print(h.po, h.po[1], h.po[1][1])
expect_err("h.po = []")
expect_err("h.po[1] = []")
a = h.po
a[1][1] = {}
print(a, a[1], a[1][1])
expect_err("a[1] = {}")
a = h.po[1]
print(a, a[1])

expect_err("print (h.foo())")
mkfoo()
h.po[0][0] = {}
print(h.foo(), h.po[0][0])
h("objref po")
h.po = []
expect_err("print(h.foo())")
expect_err("z = a[1]")
expect_err("print(a[1])")
expect_err("a[1] = []")
expect_err("len(a)")
expect_err("print(bool(a))")
