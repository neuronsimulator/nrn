from neuron import h


def check_callable(get_instance, has_voltage=True):
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

    if has_voltage:
        # Check `f` using `v`.
        expected = -2.0
        actual = get_instance(x).v_plus_a(40.0)
        assert actual == expected, f"{actual} == {expected}"


nseg = 5
s = h.Section()
s.nseg = nseg

s.insert("functions")
s.insert("non_threadsafe")

coords = [(0.5 + k) * 1.0 / nseg for k in range(nseg)]
values = [0.1 + k for k in range(nseg)]

point_processes = {x: h.point_functions(s(x)) for x in coords}
point_non_threadsafe = {x: h.point_non_threadsafe(s(x)) for x in coords}

art_cells = {x: h.art_functions() for x in coords}

check_callable(lambda x: s(x).functions)
check_callable(lambda x: s(x).non_threadsafe)
check_callable(lambda x: point_processes[x])
check_callable(lambda x: point_non_threadsafe[x])
check_callable(lambda x: art_cells[x], has_voltage=False)
