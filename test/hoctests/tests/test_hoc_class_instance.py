from neuron import h

h(
    """
begintemplate A
proc init() {
  x = 2
}
endtemplate A
"""
)


def test1():
    v = h.Vector()
    vstr = "h." + str(v)
    print(vstr)
    exec(vstr + ".resize(10)")
    assert len(v) == 10


def test2():
    a = h.A()
    assert h.A[0] == a


if __name__ == "__main__":
    test1()
    test2()
