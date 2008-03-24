"""
An attempt to make use of HocObjects more Pythonic, by wrapping them in Python
classes with the same names as the Hoc classes, and adding various typical
Python methods, such as __len__().

$Id: neuron.py 18 2007-05-18 09:08:00Z apdavison $
"""

import hoc
from hoc import HocObject

h = hoc.HocObject()
h('obfunc newlist() { return new List() }') 
h('obfunc newvec() { return new Vector($1) }')


class List(object):
    
    def __init__(self):
        self.hoc_obj = h.newlist()
        
    def __getattr__(self,name):
        return getattr(self.hoc_obj, name)
    
    def __len__(self):
        return self.count()
    
    
class Vector(object):
    
    def __init__(self,arg=10):
        if isinstance(arg,int):
            self.hoc_obj = h.newvec(arg)
        elif isinstance(arg,list):
            self.hoc_obj = h.newvec(len(arg))
            for i,x in enumerate(arg):
                self.x[i] = x
        
    def __getattr__(self,name):
        return getattr(self.hoc_obj, name)
    
    def __len__(self):
        return self.size()
    
    def __str__(self):
        tmp = self.printf()
        return ''
    
    def __repr__(self):
        tmp = self.printf()
        return ''


    # allow array(Vector())
    # Need Vector().toarray for speed though
    def __getitem__(self,i):
        return self.x[i]
    

    def __setitem__(self,i,y):
        self.x[i] = y

    def __getslice__(self,i,j):
        return [self.x[ii] for ii in xrange(i,j)]

    def __setslice__(self,i,j,y):
        assert(len(y)==j-i)

        iter = y.__iter__()

        for ii in xrange(i,j):
            self.x[ii] = iter.next()
            
        

    
    def tolist(self):
        return [self.x[i] for i in range(int(self.size()))]
            
