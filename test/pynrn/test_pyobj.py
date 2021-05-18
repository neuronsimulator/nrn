import neuron
import pytest
import sys


def test_hclass_origin():
    import neuron.hclass2, neuron.hclass3

    if sys.version_info.major == 3:
        self = neuron.hclass3
        other = neuron.hclass2
    else:
        self = neuron.hclass2
        other = neuron.hclass3
    # Confirm that our hclass was loaded by import machinery
    assert self.__spec__ is not None
    # And that the other one is a dummy
    assert other.__spec__ is None


if neuron._pyobj_enabled:
    from _pyobj_testing import *
