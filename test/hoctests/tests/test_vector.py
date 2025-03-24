from neuron import h
from neuron.expect_hocerr import expect_err, set_quiet

set_quiet(False)


def add_two(x):
    return x + 2


vec = h.Vector([4, -1.5, 17])
vec.apply(add_two)
assert list(vec) == [6.0, 0.5, 19.0]
expect_err("vec.apply(add_two, 1, 2, 5, 24, 13)")  # too many args
expect_err("vec.apply('hocfunctionthatdoesnotexist')")
expect_err("vec.apply(vec)")
expect_err("vec.apply(42)")

h("""
func subtract_two() {
    return $1 - 2
}
""")
vec.apply("subtract_two")
assert list(vec) == [4.0, -1.5, 17]


def unusable_function(x):
    # this should fail because of the return value
    return unusable_function


expect_err("vec.apply(unusable_function)")