from ._nmodl import *
from .config import *


def example_dir():
    """Returns directory containing NMODL examples

    NMODL Framework is installed with few sample example of
    channels. This method can be used to get the directory
    containing all mod files.

    Returns:
        Full path of directory containing sample mod files
    """
    import os
    installed_example = os.path.join(PROJECT_INSTALL_DIR, "share", "example")
    if os.path.exists(installed_example):
        return installed_example
    else:
        return os.path.join(PROJECT_SOURCE_DIR, "share", "example")

