import sys

if sys.version_info >= (3, 9):
    from importlib.resources import files
else:
    from importlib_resources import files

from ._nmodl import *

RESOURCE_DIR = "ext/example"


def list_examples():
    """Returns a list of examples available

    The NMODL Framework provides a few examples for testing

    Returns:
        List of available examples
    """
    path = files("nmodl") / RESOURCE_DIR
    if path.exists() and path.is_dir():
        return [result.name for result in path.glob("*.mod")]

    raise FileNotFoundError("Could not find sample directory")


def load_example(example):
    """Load an example from the NMODL examples

    The NMODL Framework provides a few examples for testing. The list of examples can be requested
    using `list_examples()`. This function then returns the NMODL code of the requested example
    file.

    Args:
        example: Filename of an example as provided by `list_examples()`
    Returns:
        An path to the example as a string
    """
    path = files("nmodl") / RESOURCE_DIR / example
    if path.exists():
        return path.read_text()

    raise FileNotFoundError(f"Could not find sample mod file {example}")
