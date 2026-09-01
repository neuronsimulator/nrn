import numpy as np


def test_invalid_arith_objects(neuron_nosave_instance):
    """Test Invalid Arithmeticed objects"""

    h, rxd, _ = neuron_nosave_instance

    try:
        rxd.rxdmath._ensure_arithmeticed(None)
    except Exception as e:
        assert isinstance(e, rxd.RxDException)

    try:
        rxd.rxdmath._ensure_arithmeticed("hello")
    except Exception as e:
        assert isinstance(e, rxd.RxDException)

    try:
        rxd.rxdmath._ensure_arithmeticed([])
    except Exception as e:
        assert isinstance(e, rxd.RxDException)

    try:
        rxd.rxdmath._ensure_arithmeticed([1, 2])
    except Exception as e:
        assert isinstance(e, rxd.RxDException)

    try:
        rxd.rxdmath._ensure_arithmeticed(1 + 1j)
    except Exception as e:
        assert isinstance(e, rxd.RxDException)


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
