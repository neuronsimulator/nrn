from pathlib import Path

from ._nmodl import *

RESOURCE_DIR = "ext/example"


def list_examples():
    """Returns a list of examples available

    The NMODL Framework provides a few examples for testing

    Returns:
        List of available examples
    """
    path = Path(__file__).parent / RESOURCE_DIR
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
    path = Path(__file__).parent / RESOURCE_DIR / example
    if path.exists():
        return path.read_text()

    raise FileNotFoundError(f"Could not find sample mod file {example}")
