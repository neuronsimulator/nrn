from neuron import h


def check_defaults(obj, parameters):
    defaults = {
        "b": 0.1,
        "y": 2.1,
    }

    for p in parameters:
        actual = getattr(obj, f"{p}_default_parameter")
        expected = defaults.get(p, 0.0)
        assert actual == expected


def test_default_values():
    nseg = 10000

    s = h.Section()
    s.nseg = nseg
    s.insert("default_parameter")

    static_parameters = ["a", "b"]
    range_parameters = ["x", "y", "z"]

    check_defaults(h, static_parameters)
    for seg in s:
        check_defaults(seg, range_parameters)

    h.finitialize()

    check_defaults(h, static_parameters)
    for seg in s:
        check_defaults(seg, range_parameters)


def test_parcom_regression():
    # This is a roundabout way of triggering a codepath leading to
    # NrnProperyImpl. In NrnProperyImpl we iterate over the std::vector of
    # default values. Hence, it requires that the default values are registered
    # correctly.

    nseg = 5

    s = h.Section()
    s.nseg = nseg
    s.insert("default_parameter")

    # Caused a SEGFAULT:
    h.load_file("parcom.hoc")


if __name__ == "__main__":
    test_default_values()
    test_parcom_regression()
