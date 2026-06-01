from pathlib import Path

import pytest
from neuron import nmodl


def test_parse_directory():
    """
    Make sure we raise an error when parsing a directory instead of a file
    """

    with pytest.raises(RuntimeError):
        nmodl.NmodlDriver().parse_file(str(Path(__file__).parent))
