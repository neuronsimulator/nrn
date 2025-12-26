# release/8.2 problem with aliases in LinearCircuit builder #3688

from neuron import h

h(
    r"""
begintemplate TstAlias
public a, b
proc init() {
    a = 5
    b = 1
}
endtemplate TstAlias
"""
)


def test_alias():
    a = h.TstAlias()
    sf = h.StringFunctions()
    sf.alias(a, "V_out", a._ref_a)
    sf.alias(a, "V_out_0", a._ref_b)
    print(f"a.a={a.a} a.V_out={a.V_out}")
    print(f"a.b={a.b} a.V_out_0={a.V_out_0}")
    assert a.V_out == a.a


if __name__ == "__main__":
    test_alias()
