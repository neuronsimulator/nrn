# This lives in test/hoctests so that it is run as a plain Python script,
# outside pytest. If neuron is already imported when this test is run then it
# will not work. This test trivially increases NFRAME by a small amount and
# tests that that increase is taken into account. When this test was written,
# default NSTACK=1000 and NFRAME=512
import os
import pytest
import sys


def test_neuronoptions():
    if "neuron" in sys.modules:
        raise Exception("neuron must not already be imported")

    new_framesize = 525
    nrnoptions = "-nogui -NSTACK 3000 -NFRAME " + str(new_framesize)
    os.environ["NEURON_MODULE_OPTIONS"] = nrnoptions

    from neuron import h

    # test that HOC NFRAME is actually larger than the default 512
    h(
        """
    func add_recurse() {
    if ($1 == 0) {
        return 0
    }
    return $1 + add_recurse($1-1)
    }
    """
    )

    # First token is NEURON and may have -dll option
    nrnver7 = h.nrnversion(7)
    assert nrnoptions in nrnver7
    i = new_framesize - 5
    assert h.add_recurse(i) == i * (i + 1) / 2
    with pytest.raises(RuntimeError):
        i = new_framesize
        h.add_recurse(i) == i * (i + 1) / 2


if __name__ == "__main__":
    sys.exit(pytest.main([__file__]))
