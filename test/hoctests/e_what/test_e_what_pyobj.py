# Python->HOC PythonObject attr/index and a non-floatable arg must raise, not abort.
from neuron import h
from neuron.expect_hocerr import expect_err, set_quiet

set_quiet(False)


class Box:
    data = [1]

    def __call__(self):
        pass


def test_getitem_and_attr():
    h("func e_what_ok() { return $o1.data[0] }")
    assert h.e_what_ok(Box()) == 1.0
    h("func e_what_oob() { return $o1.data[90] }")
    expect_err("h.e_what_oob(Box())")
    h("func e_what_miss() { return $o1.nope }")
    expect_err("h.e_what_miss(Box())")


def test_nonfloatable_hoc_arg():
    expect_err("h.exp(1j)")


if __name__ == "__main__":
    test_getitem_and_attr()
    test_nonfloatable_hoc_arg()
