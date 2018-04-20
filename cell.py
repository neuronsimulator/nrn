# -*- coding: utf-8 -*-
r"""
Generic NEURON cell object. Provides template abstract methods and a generic 
design for NEURON cell models. 

AUTHORS:
- THOMAS MCTAVISH (2010-11-04).

modified to use h instead of nrn, removed line continuation characters -- 2015-04-21
"""

import numpy
from neuron import h

class Cell:
    """Generic cell template."""
    
    def __init__(self):
        self.x = 0; self.y = 0; self.z = 0
        self.soma = None
        self.synlist = [] #### NEW CONSTRUCT IN THIS WORKSHEET
        self.create_sections()
        self.build_topology()
        self.build_subsets()
        self.define_geometry()
        self.define_biophysics()
        self.create_synapses()
        
    def create_sections(self):
        """Create the sections of the cell. Remember to do this
        in the form::
            
            h.Section(name='soma', cell=self)
            
        """
        raise NotImplementedError("create_sections() is not implemented.")
        
    def build_topology(self):
        """Connect the sections of the cell to build a tree."""
        raise NotImplementedError("build_topology() is not implemented.")
        
    def define_geometry(self):
        """Set the 3D geometry of the cell."""
        raise NotImplementedError("define_geometry() is not implemented.")

    def define_biophysics(self):
        """Assign the membrane properties across the cell."""
        raise NotImplementedError("define_biophysics() is not implemented.")
        
    def create_synapses(self):
        """Subclasses should create synapses (such as ExpSyn) at various
        segments and add them to self.synlist."""
        pass # Ignore if child does not implement.
        
    def build_subsets(self):
        """Build subset lists. This defines 'all', but subclasses may
        want to define others. If overriden, call super() to include 'all'."""
        self.all = h.SectionList()
        self.all.wholetree(sec=self.soma)
        
    def connect2target(self, target, thresh=10):
        """Make a new NetCon with this cell's membrane
        potential at the soma as the source (i.e. the spike detector)
        onto the target passed in (i.e. a synapse on a cell).
        Subclasses may override with other spike detectors."""
        nc = h.NetCon(self.soma(1)._ref_v, target, sec = self.soma)
        nc.threshold = thresh
        return nc
    
    def is_art(self):
        """Flag to check if we are an integrate-and-fire artificial cell."""
        return 0
        
    def set_position(self, x, y, z):
        """
        Set the base location in 3D and move all other
        parts of the cell relative to that location.
        """
        for sec in self.all:
            for i in range(sec.n3d()):
                h.pt3dchange(i, 
                        x-self.x+sec.x3d(i), 
                        y-self.y+sec.y3d(i), 
                        z-self.z+sec.z3d(i), 
                        sec.diam3d(i), sec=sec)
        self.x = x; self.y = y; self.z = z

    def rotateZ(self, theta):
        """Rotate the cell about the Z axis."""
        rot_m = numpy.array([[numpy.sin(theta), numpy.cos(theta)], 
                [numpy.cos(theta), -numpy.sin(theta)]])
        for sec in self.all:
            for i in range(sec.n3d()):
                xy = numpy.dot([sec.x3d(i), sec.y3d(i)], rot_m)
                h.pt3dchange(i, float(xy[0]), float(xy[1]), sec.z3d(i), 
                        sec.diam3d(i))
