#Python 3 only
# ------------------------------------------------------------------------------
# class factory for subclassing h.anyclass
# h.anyclass methods may be overridden. If so the base method can be called
# using the idiom self.basemethod = self.baseattr('methodname')
# ------------------------------------------------------------------------------

from neuron import h, hoc
import sys, nrn

def assert_not_hoc_composite(cls):
    hoc_bases = [b for b in cls.__bases__ if issubclass(b, hoc.HocObject)]
    if len(hoc_bases) > 1:
        bases = ", ".join(b.__name__)
        cname = cls.__name__
        raise TypeError(f"Composition of {bases} HocObjects not allowed in {cname}")

def hclass(hoc_template):
    """Class factory for subclassing h.anyclass. E.g. class MyList(hclass(h.List)):..."""
    if hoc_template == h.Section:
        return nrn.Section

    class hc(hoc.HocObject):
        htype = hoc_template

        def __init_subclass__(cls, **kwargs):
            assert_not_hoc_composite(cls)
            super().__init_subclass__(**kwargs)

        def __new__(cls, *args, **kwds):
            kwds2 = {'hocbase': cls.htype}
            if 'sec' in kwds:
                kwds2['sec'] = kwds['sec']
            return hoc.HocObject.__new__(cls, *args, **kwds2)

    return hc

def nonlocal_hclass(hoc_template, module_name, name=None):
    module = sys.modules[module_name]
    if name is None:
        name = hoc_template.hname()[:-2]
    local_hclass = hclass(hoc_template)
    setattr(module, name, local_hclass)
    local_hclass.__module__ = module.__name__
    local_hclass.__name__ = name
    local_hclass.__qualname__ = name
    return local_hclass
