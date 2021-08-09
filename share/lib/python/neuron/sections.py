"""
Extensions of the nrn.Section class.

$Id:$
"""

import neuron
from neuron import nrn
import sys
import io


class ExtendedSection(nrn.Section):
    def __init__(self, name=None):
        nrn.Section.__init__(self)
        self.hoc_name = nrn.Section.name(self)
        if name:
            self._name = name
        else:
            self._name = self.hoc_name
        self.mechanisms = set()

    def __getattr__(self, name):
        if name in self.mechanisms:
            return AllSegments(self, name)
        else:
            return self.__getattribute__(name)

    def insert(self, mech_name):
        self.mechanisms.add(mech_name)
        return nrn.Section.insert(self, mech_name)

    def _get_name(self):
        return self._name

    def _set_name(self, name):
        assert isinstance(name, str)
        self._name = name

    name = property(fget=_get_name, fset=_set_name)

    def psection(self):
        if self.name != self.hoc_name:
            sys.stdout.write("%s:  " % self.name)
        neuron.psection(self)


class AllSegments(object):
    """Allows setting membrane properties for all segments in a section at once,
    without having to explicitly iterate over all segments.

    Example usage:
        >>> soma = neuron.sections.ExtendedSection()
        >>> soma.nseg = 3
        >>> soma.insert('pas')
        >>> type(soma.pas)
        <class 'neuron.sections.AllSegments'>
        >>> soma.pas.e = -72
        >>> soma.pas.e
        [-72.0, -72.0, -72.0]
        >>> soma(0.5).pas.e
        -72.0
    """

    def __init__(self, sec, mech_name):
        self.sec = sec
        self.mech_name = mech_name

    def __setattr__(self, name, value):
        success = False
        for seg in self.sec:
            if hasattr(seg, self.mech_name):
                mech = getattr(seg, self.mech_name)
                if hasattr(mech, name):
                    mech.name = value
                    success = True
        if not success:
            try:
                object.__setattr__(self, name, value)
            except AttributeError:
                raise AttributeError(
                    "%s mechanism has no attribute %s" % (self.mech_name, name)
                )

    def __getattr__(self, name):
        try:
            return self.__getattribute__(name)
        except AttributeError:
            return [
                getattr(getattr(seg, self.mech_name), name)
                for seg in self.sec
                if hasattr(seg, self.mech_name)
            ]
