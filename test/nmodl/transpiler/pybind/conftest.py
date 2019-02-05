import pytest
import nmodl

CHANNEL = """NEURON  {
    SUFFIX NaTs2_t
    RANGE mInf, hInf
}

ASSIGNED {
    mInf
    hInf
}

STATE   {
    m
    h
}

DERIVATIVE states   {
    m' = mInf-m
    h' = hInf-h
}
"""


@pytest.fixture
def ch_ast():
    d = nmodl.Driver()
    d.parse_string(CHANNEL)
    return d.ast()