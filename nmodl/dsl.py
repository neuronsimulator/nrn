from ._nmodl import *
from .config import *


def example_dir():
    import os
    """Return directory containing NMODL examples"""
    installed_example = os.path.join(PROJECT_INSTALL_DIR, "share", "example")
    if os.path.exists(installed_example):
        return installed_example
    else:
        return os.path.join(PROJECT_SOURCE_DIR, "share", "example")

