# -*- coding: utf-8 -*-
r"""
Ball and stick model. This is a two-section cell: A soma with active channels 
and a dendrite with passive properties.

AUTHORS:
- THOMAS MCTAVISH (2010-11-04): initial version. Modified version of the
        project by Hines and Carnevale. (Hines M.L. and Carnevale N.T, 
        Translating network models to parallel hardware in NEURON,
        Journal of Neuroscience Methods 169 (2008) 425-455).

modified to use h instead of nrn -- 2015-04-21
"""
from cell import *
from neuron import h

class BallAndStick(Cell):  #### Inherits from Cell
    """Two-section cell: A soma with active channels and
    a dendrite with passive properties."""
    
    #### __init__ is gone and handled in Cell.
    #### We can override __init__ completely, or do some of
    #### our own initialization first, and then let Cell do its
    #### thing, and then do a bit more ourselves with "super".
    ####
    #### def __init__(self):
    ####     # Do some stuff
    ####     super(Cell, self).__init__()
    ####     # Do some more stuff
             
    def create_sections(self):
        """Create the sections of the cell."""
        self.soma = h.Section(name='soma', cell=self)
        self.dend = h.Section(name='dend', cell=self)
        
    def build_topology(self):
        """Connect the sections of the cell to build a tree."""
        self.dend.connect(self.soma(1))
        
    def define_geometry(self):
        """Set the 3D geometry of the cell."""
        self.soma.L = self.soma.diam = 12.6157 # microns
        self.dend.L = 200                      # microns
        self.dend.diam = 1                     # microns
        self.dend.nseg = 5
        self.shape_3D()

    def define_biophysics(self):
        """Assign the membrane properties across the cell."""
        for sec in self.all: # 'all' exists in parent object.
            sec.Ra = 100    # Axial resistance in Ohm * cm
            sec.cm = 1      # Membrane capacitance in micro Farads / cm^2
        
        # Insert active Hodgkin-Huxley current in the soma
        self.soma.insert('hh')
        self.soma.gnabar_hh = 0.12  # Sodium conductance in S/cm2
        self.soma.gkbar_hh = 0.036  # Potassium conductance in S/cm2
        self.soma.gl_hh = 0.0003    # Leak conductance in S/cm2
        self.soma.el_hh = -54.3     # Reversal potential in mV
        
        # Insert passive current in the dendrite
        self.dend.insert('pas')
        self.dend.g_pas = 0.001  # Passive conductance in S/cm2
        self.dend.e_pas = -65    # Leak reversal potential mV
    
    def shape_3D(self):
        """
        Set the default shape of the cell in 3D coordinates.
        Set soma(0) to the origin (0,0,0) and dend extending along
        the X-axis.
        """
        len1 = self.soma.L
        self.soma.push()
        h.pt3dclear()
        h.pt3dadd(0, 0, 0, self.soma.diam)
        h.pt3dadd(len1, 0, 0, self.soma.diam)
        h.pop_section()
        
        len2 = self.dend.L
        self.dend.push()
        h.pt3dclear()
        h.pt3dadd(len1, 0, 0, self.dend.diam)
        h.pt3dadd(len1 + len2, 0, 0, self.dend.diam)
        h.pop_section()
        
    #### build_subsets, rotateZ, and set_location are now in cell object. ####
    
    #### NEW STUFF ####
    
    def create_synapses(self):
        syn = h.ExpSyn(self.dend(0.5))
        syn.tau = 2
        self.synlist.append(syn) # synlist is defined in Cell
