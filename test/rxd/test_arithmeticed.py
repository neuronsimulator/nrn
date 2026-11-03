import numpy as np
import pytest


def test_invalid_arith_objects(neuron_nosave_instance):
    """Test Invalid Arithmeticed objects"""

    h, rxd, _ = neuron_nosave_instance

    for invalid in (None, "hello", [], [1, 2], 1 + 1j):
        with pytest.raises(rxd.RxDException):
            rxd.rxdmath._ensure_arithmeticed(invalid)


def test_valid_arith_objects(neuron_nosave_instance):
    """Test Valid Arithmeticed objects"""

    h, rxd, _ = neuron_nosave_instance
    assert isinstance(rxd.rxdmath._ensure_arithmeticed(1), rxd.rxdmath._Arithmeticed)
    assert isinstance(
        rxd.rxdmath._ensure_arithmeticed(np.float32(1.5)), rxd.rxdmath._Arithmeticed
    )
    assert isinstance(
        rxd.rxdmath._ensure_arithmeticed(3 / 2), rxd.rxdmath._Arithmeticed
    )
    dend = h.Section("dend")
    cyt = rxd.Region(dend.wholetree(), nrn_region="i")
    ca = rxd.Species(cyt, name="ca", charge=2, initial=1e-12)
    assert isinstance(
        rxd.rxdmath._ensure_arithmeticed((ca * (1 - ca))), rxd.rxdmath._Arithmeticed
    )
