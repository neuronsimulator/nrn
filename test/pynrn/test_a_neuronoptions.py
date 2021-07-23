# test_a_neuronoptions.py named to be first test file in this folder.
# This is so testing with pytest will do this first. Otherwise
# neuron will already be imported and this test will not be substantive
# as it relies on being the first code to 'import neuron'. If neuron is
# already imported, a warning will be raised.
# This test trivially increases NFRAME by a small amount and tests
# that increase. Should not affect other tests in this folder.
# When this test was written, default NSTACK=1000 and NFRAME=512
#


import sys, warnings

neuron_already_imported = "neuron" in sys.modules

import os

nrnoptions = "-nogui -NSTACK 3000 -NFRAME 525"
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


def test_neuronoptions():
    nrnver7 = h.nrnversion(7)  # First token is NEURON and may have -dll option
    print(nrnoptions)
    print(nrnver7)
    if neuron_already_imported:
        warnings.warn(
            UserWarning("neuron was already imported prior to setting os.environ")
        )
    else:
        assert nrnoptions in nrnver7
        i = 520
        assert h.add_recurse(i) == i * (i + 1) / 2


if __name__ == "__main__":
    test_neuronoptions()
