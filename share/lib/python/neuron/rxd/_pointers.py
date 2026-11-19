"""Plain C pointers for rxd's C code, which lives in libnrniv and takes no Python objects."""

import ctypes
import numbers
from typing import Optional

from neuron import hoc

# a C double** argument
ptr_array_type = ctypes.POINTER(ctypes.c_void_p)


def address(ref) -> int:
    """The address of the double that a reference such as seg._ref_v holds."""
    return hoc.ref_address(ref)


def ptr_array(refs) -> Optional[ctypes.Array]:
    """The addresses of refs as a C array of pointers, or None if there are none."""
    if not refs:
        return None
    return (ctypes.c_void_p * len(refs))(*[address(ref) for ref in refs])


def ptr_or_value(x) -> tuple:
    """(address, 0.0) for a reference, (None, x) for a number, for C that takes either."""
    if isinstance(x, numbers.Real):
        return None, float(x)
    return address(x), 0.0
