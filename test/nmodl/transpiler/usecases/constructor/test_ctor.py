import gc

from neuron import h, gui


def allocate_props(n_instances, mech_name):
    s = h.Section()
    s.nseg = 1

    for _ in range(n_instances):
        getattr(h, mech_name)(s(0.5))


def assert_equal(actual, expected):
    assert actual == expected, f"{actual} != {expected}"


def check_ctor(n_instances, mech_name):
    allocate_props(n_instances, mech_name)
    gc.collect()

    assert_equal(getattr(h, f"ctor_calls_{mech_name}"), n_instances)
    assert_equal(getattr(h, f"dtor_calls_{mech_name}"), n_instances)


def test_ctor():
    check_ctor(12, "ctor")


def test_art_ctor():
    check_ctor(32, "art_ctor")


if __name__ == "__main__":
    test_ctor()
    test_art_ctor()
