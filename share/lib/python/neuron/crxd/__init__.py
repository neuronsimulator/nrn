import sys
import neuron.rxd

# neuron.crxd is another name for neuron.rxd. Alias the submodules too: otherwise
# "from neuron.crxd.rxdmath import ..." loads a second copy, with its own classes.
for _name, _module in list(sys.modules.items()):
    if _name.startswith("neuron.rxd."):
        sys.modules["neuron.crxd" + _name[len("neuron.rxd") :]] = _module

sys.modules[__name__] = sys.modules["neuron.rxd"]
