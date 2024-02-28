from json import loads
from pathlib import Path

from nmodl import dsl


def test_example():
    """
    Test for the Python API from example
    """

    examples = dsl.list_examples()

    # ordering may be off so we use a set
    assert set(examples) == {"exp2syn.mod", "expsyn.mod", "hh.mod", "passive.mod"}

    driver = dsl.NmodlDriver()
    for example in examples:
        nmodl_string = dsl.load_example(example)
        # make sure we can parse the string
        assert driver.parse_string(nmodl_string)
