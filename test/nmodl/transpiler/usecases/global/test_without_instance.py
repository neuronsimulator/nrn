from neuron import h, gui


def make_accessors(mech_name, read_only):
    get = getattr(h, f"get_gbl_{mech_name}")

    if read_only:
        return get, None

    set = getattr(h, f"set_gbl_{mech_name}")
    return get, set


def check_write_read_cycle(mech_name, read_only=False):
    get, set = make_accessors(mech_name, read_only)

    if read_only:
        expected = 42.0
    else:
        expected = 278.045
        set(expected)

    actual = get()
    assert (
        actual == expected
    ), f"{actual = }, {expected = }, delta = {actual - expected}"


def test_top_local():
    check_write_read_cycle("top_local")


def test_global():
    check_write_read_cycle("global")


def test_parameter():
    check_write_read_cycle("parameter", True)


if __name__ == "__main__":
    test_top_local()
    test_global()
    test_parameter()
