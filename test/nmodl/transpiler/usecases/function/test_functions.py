from neuron import h


def check_functions(get_instance):
    for x, value in zip(coords, values):
        get_instance(x).x = value

    for x, value in zip(coords, values):
        expected = 100.0 + value
        actual = get_instance(x).x_plus_a(100.0)
        assert actual == expected, f"{expected} != {actual}"

    x = coords[0]
    v0 = -42.0
    h.finitialize(v0)

    # Check `f(v)`.
    expected = 42.0
    actual = get_instance(x).identity(expected)
    assert actual == expected, f"{actual} == {expected}"

    # Check `f` using `v`.
    expected = -2.0
    actual = get_instance(x).v_plus_a(40.0)
    assert actual == expected, f"{actual} == {expected}"


nseg = 5
s = h.Section()
s.nseg = nseg

s.insert("functions")

coords = [(0.5 + k) * 1.0 / nseg for k in range(nseg)]
values = [0.1 + k for k in range(nseg)]

point_processes = {x: h.point_functions(s(x)) for x in coords}

check_functions(lambda x: s(x).functions)
check_functions(lambda x: point_processes[x])
