#Python2 only
# ------------------------------------------------------------------------------
# class factory for subclassing h.anyclass
# h.anyclass methods may be overridden. If so the base method can be called
# using the idiom self.basemethod = self.baseattr('methodname')
# ------------------------------------------------------------------------------
      
from neuron import h, hoc

class MetaHocObject(type):
  """Provides Exception for Inheritance of multiple HocObject"""
  def __new__(cls, name, bases, attrs):
    #print cls, name, bases
    m = []
    for b in bases:
      if issubclass(b, hoc.HocObject):
        m.append(b.__name__)
    if (len(m) > 1):
      raise TypeError('Multiple Inheritance of HocObject in %s' % name
        + ' through %s not allowed' % ','.join(m))
      #note that join(b.__name__ for b in m) is not valid for Python 2.3
  
    return type.__new__(cls, name, bases, attrs)

def hclass(c):
    """Class factory for subclassing h.anyclass. E.g. class MyList(hclass(h.List)):..."""
    if c == h.Section :
        return nrn.Section
    #class hc(hoc.HocObject, metaclass=MetaHocObject):
    class hc(hoc.HocObject):
        def __new__(cls, *args, **kwds):
            kwds2 = {'hocbase': cls.htype}
            if 'sec' in kwds:
                kwds2['sec'] = kwds['sec']
            return hoc.HocObject.__new__(cls, *args, **kwds2)
    setattr(hc, 'htype', c)
    return hc
