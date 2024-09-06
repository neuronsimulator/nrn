from neuron import h
import re

h(
    """
begintemplate A
proc init() {
  x = 2
}
endtemplate A
"""
)


def extract_index(s):
    match = re.search(r"\[(\d+)\]", s)
    if match:
        return int(match.group(1))
    else:
        raise ValueError("String does not match the expected format")


def test1():
    v = h.Vector()
    assert bool(v) is False
    vstr = "h." + str(v)
    print(vstr)
    exec(vstr + ".resize(10)")
    assert len(v) == 10
    assert bool(v) is True
    ix = extract_index(str(v))
    assert h.Vector[ix] == v
    assert type(v).__module__ == "hoc"


def test2():
    a = h.A()
    assert h.A[0] == a


if __name__ == "__main__":
    test1()
    test2()
