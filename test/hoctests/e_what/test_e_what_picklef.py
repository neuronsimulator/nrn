# ParallelContext.submit of a raising Python callable must not abort (call_picklef).
from neuron import h
from neuron.expect_hocerr import expect_err, set_quiet

set_quiet(False)

pc = h.ParallelContext()


def ident(x):
    return x


def boom(x):
    raise RuntimeError("picklef boom")


def test_call_picklef_raise():
    pc.runworker()
    pc.submit(ident, 3.0)
    assert pc.working()
    assert pc.pyret() == 3.0
    pc.submit(boom, 1.0)
    expect_err("pc.working()")
    pc.done()


if __name__ == "__main__":
    test_call_picklef_raise()
