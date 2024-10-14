from neuron import h, gui


def test_nonlin():
    s = h.Section()
    s.insert("nonlin")
    inst = s(0.5).nonlin

    x = inst.solve()
    error = inst.residual(x)
    assert abs(error) < 1e-9, f"{x = }, {error = }"


if __name__ == "__main__":
    test_nonlin()
