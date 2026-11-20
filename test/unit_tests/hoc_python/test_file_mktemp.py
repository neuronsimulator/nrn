"""File.mktemp must create the file, not only reserve a name."""

import os

from neuron import h


def test_file_mktemp_creates_distinct_files():
    """Two calls leave two existing files. A name-only mktemp would not."""
    first = h.File()
    assert first.mktemp()
    name1 = str(first.getname())
    second = h.File()
    assert second.mktemp()
    name2 = str(second.getname())
    try:
        assert name1 != name2
        assert os.path.isfile(name1)
        assert os.path.isfile(name2)
    finally:
        first.unlink()
        second.unlink()
