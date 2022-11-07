from neuron import h
from neuron.expect_hocerr import expect_err


def test_template_err():

    h(
        """
begintemplate TestTemplateErr1
proc init() {
    create soma
}
endtemplate TestTemplateErr1
    """
    )

    expect_err("m = h.TestTemplateErr1()")

    h(
        """
begintemplate TestTemplateErr2
proc init() {
}
proc setfoo() {
    foo = 5 // 0 default if not called instead of UNDEF
}
endtemplate TestTemplateErr2
    """
    )

    m = h.TestTemplateErr2()
    expect_err('h.execute("create foo", m)')
    assert m.foo == 0.0


def test_template_err2():
    h(
        """
proc test_template_err2() {
    create test_template_err_sec
}
    """
    )
    h.test_template_err2()
    s = h.test_template_err_sec
    s.nseg = 3
    s.insert("hh")
    h.topology()
    print(s.psection())
    h.delete_section(sec=h.test_template_err_sec)


if __name__ == "__main__":
    test_template_err()
    test_template_err2()
