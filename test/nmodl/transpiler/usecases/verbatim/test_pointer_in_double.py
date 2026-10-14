from neuron import h, gui

# Since a double is 64-bits and a pointer is also 32 or 64-bits, we can store
# the bits of a pointer in the memory that was allocated for use as doubles.
# Concretely, we, using VERBATIM, safe pointers in ASSIGNED or RANGE variables.
#
# The pattern is found in the builtin mod file: `apcount.mod`.


def test_pointer_in_double():
    s = h.Section()
    s.nseg = 3
    s.insert("pointer_in_double")

    h.finitialize()
    for i, x in enumerate([0.25, 0.5, 0.75]):
        actual = s(x).pointer_in_double.use_pointer()
        expected = i
        assert actual == expected, f"{actual} != {expected}"


if __name__ == "__main__":
    test_pointer_in_double()
