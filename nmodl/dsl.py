import os.path as osp
from pkg_resources import *

from ._nmodl import *

RESOURCE_DIR = "ext/example"

def list_examples():
    """Returns a list of examples available

    The NMODL Framework provides a few examples for testing

    Returns:
        List of available examples
    """
    if resource_exists(__name__, RESOURCE_DIR) and resource_isdir(__name__, RESOURCE_DIR):
        return resource_listdir(__name__, RESOURCE_DIR)
    else:
        raise FileNotFoundError("Could not find sample directory")


def load_example(example):
    """Load an example from the NMODL examples

    The NMODL Framework provides a few examples for testing. The list of examples can be requested
    using `list_examples()`. This function then returns the NMODL code of the requested example
    file.

    Args:
        example: Filename of an example as provided by `list_examples()`
    Returns:
        List of available examples
    """
    resource =  osp.join(RESOURCE_DIR, example)
    if resource_exists(__name__, resource):
        return resource_string(__name__, resource)
    else:
        raise FileNotFoundError("Could not find sample mod files")
