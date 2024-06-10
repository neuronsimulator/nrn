from neuron import h


def check_procedures(get_instance):
    for x in coords:
        get_instance(x).set_x_42()
        assert get_instance(x).x == 42

    for x, value in zip(coords, values):
        get_instance(x).set_x_a(value)

    for x, value in zip(coords, values):
        assert get_instance(x).x == value

    x = coords[0]
    v0 = -42.0
    h.finitialize(v0)

    # Check `x = v`.
    get_instance(x).set_x_v()
    actual = get_instance(x).x
    expected = v0

    assert actual == expected, f"{actual} == {expected}"

    # Check `f(v)`.
    expected = 42.0
    actual = get_instance(x).identity(expected)
    assert actual == expected, f"{actual} == {expected}"

    # Check g = lambda: f(v)
    get_instance(x).set_x_just_v()
    actual = get_instance(x).x
    expected = v0
    assert actual == expected, f"{actual} == {expected}"

    # Check g = lambda v: f(v)
    expected = 42.0
    get_instance(x).set_x_just_vv(expected)
    actual = get_instance(x).x
    assert actual == expected, f"{actual} == {expected}"


nseg = 5
s = h.Section()
s.nseg = nseg

s.insert("procedures")

coords = [(0.5 + k) * 1.0 / nseg for k in range(nseg)]
values = [0.1 + k for k in range(nseg)]

point_processes = {x: h.point_procedures(s(x)) for x in coords}

check_procedures(lambda x: s(x).procedures)
check_procedures(lambda x: point_processes[x])
