#!/usr/bin/env python
"""
A generic wrapper to access nrn binaries from a python installation
Plese create a softlink with the binary name to be called.
"""
import os
import subprocess as sp
import pkg_resources
import sys


def _launch_command(exe_name):
    exe_path = pkg_resources.resource_filename('neuron', '.data/bin/' + exe_name)
    sp.call([exe_path] + sys.argv[1:])


if __name__ == '__main__':
    _launch_command(os.path.basename(sys.argv[0]))
