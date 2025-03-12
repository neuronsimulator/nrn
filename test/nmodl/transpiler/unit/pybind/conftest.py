# Copyright 2023 Blue Brain Project, EPFL.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: Apache-2.0

import pytest
from nmodl.dsl import NmodlDriver

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
    d = NmodlDriver()
    d.parse_string(CHANNEL)
    return d.get_ast()
