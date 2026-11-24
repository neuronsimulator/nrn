# seg.vext[i] reaches extracellular layer i from Python, as vext[i] does in HOC.
# The Python lookup used to return layer 0 for every index.
from neuron import h


def hoc_vext(i):
    h(f"vext_layer_value = vext_layers.vext[{i}](0.5)")
    return h.vext_layer_value


def test_vext_layers():
    old = int(h.nlayer_extracellular())
    h.nlayer_extracellular(2)
    try:
        h("create vext_layers")
        h("vext_layers insert extracellular")
        seg = h.vext_layers(0.5)

        seg.vext[1] = 2.5
        assert hoc_vext(0) == 0
        assert hoc_vext(1) == 2.5

        h("vext_layers.vext[0](0.5) = -1.5")
        assert seg.vext[0] == -1.5
        assert seg.vext[1] == 2.5

        ref = seg._ref_vext[1]
        assert ref[0] == 2.5
        ref[0] = 4.0
        assert hoc_vext(1) == 4.0
        assert hoc_vext(0) == -1.5
    finally:
        h("vext_layers delete_section()")
        h.nlayer_extracellular(old)


if __name__ == "__main__":
    test_vext_layers()
