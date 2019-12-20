#!/usr/bin/env python
"""
A generic wrapper to access nrn binaries from a python installation
Plese create a softlink with the binary name to be called.
"""
import os
import site
import subprocess as sp
import sys


def _launch_command(exe_name):
    NRN_PREFIX = os.path.join(site.getsitepackages()[0], 'neuron', '.data')
    os.environ["NEURONHOME"] = os.path.join(NRN_PREFIX, 'share/nrn')
    exe_path = os.path.join(NRN_PREFIX, 'bin', exe_name)
    return sp.call([exe_path] + sys.argv[1:])


if __name__ == '__main__':
    sys.exit(_launch_command(os.path.basename(sys.argv[0])))
