"""CoreNEURON's hooks: neuron.coreneuron's enable, file_mode and nrncore_arg.

pc.psolve asks the core whether to run CoreNEURON, and for its argument string.
As NEURON's, the hooks read the "neuron.coreneuron" module only if it has been
imported; the core prints an error and sees CoreNEURON as disabled.
"""

import sys

_MODULE = "neuron.coreneuron"


def enable_value():
    module = sys.modules.get(_MODULE)
    return 0 if module is None else int(module.enable)


def file_mode_value():
    module = sys.modules.get(_MODULE)
    return 0 if module is None else int(module.file_mode)


def nrncore_arg(tstop):
    """neuron.coreneuron.nrncore_arg(tstop); None without the module or if empty."""
    module = sys.modules.get(_MODULE)
    if module is None:
        return None
    arg = module.nrncore_arg(tstop)
    if not isinstance(arg, str) or not arg.isascii():
        raise TypeError("neuron.coreneuron.nrncore_arg() must return an ascii string")
    return arg or None
