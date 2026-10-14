from neuron import h, gui


def check_solve(make_inst):
    s = h.Section()
    s.insert("nonlin")

    inst = make_inst(s)

    x = inst.solve()
    error = inst.residual(x)
    assert abs(error) < 1e-9, f"{x = }, {error = }"


def test_nonlin():
    check_solve(lambda s: s(0.5).nonlin)


def test_art_nonlin():
    check_solve(lambda s: h.art_nonlin())


if __name__ == "__main__":
    test_nonlin()
    test_art_nonlin()
