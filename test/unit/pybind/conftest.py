# ***********************************************************************
# Copyright (C) 2018-2022 Blue Brain Project
#
# This file is part of NMODL distributed under the terms of the GNU
# Lesser General Public License. See top-level LICENSE file for details.
# ***********************************************************************

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
