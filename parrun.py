# -*- coding: utf-8 -*-
r"""
Run a parallel ring network of ball-and-stick cells and write the spike times
to a file.

To execute with MPI across 2 processors::
       
    $ mpiexec -n 2 python parrun.py
    
To modify any simulation parameters, specify individual variables encased in
a string to a sim_var dictionary variable::

    $ mpiexec -n 4 python parrun.py "sim_var['N']=6" \
       "sim_var['spike_out_file']='N6.spk'"
    
AUTHORS:

- THOMAS MCTAVISH (2010-11-04): initial version

Robert A McDougal (2015-04-21): added missing sys import
Robert A McDougal (2018-04-20): Python 3 compatibility fixes, removed "nrn"
"""
import os
import numpy
from mpi4py import MPI # Must come before importing NEURON
from neuron import h
from ring import *
import sys

h.load_file("stdrun.hoc") # For when we run simulations

def process_args(in_argv):
    """Process additional arguments on command line. Any Python statement can
    be executed, but strings in the form ``"sim_var['<var>']=x"`` will run
    the simulation with a modification of parameters."""
    sim_var = {'N' : 5, # Number of cells
            'stim_w' : 0.004, # Stimulus weight
            'stim_spike_num' : 1, # Number of spikes in the stimulator
            'syn_w' : 0.01, # Synaptic weights
            'syn_delay' : 5, # Synaptic delay
            'spike_out_file' : 'out.spk' # Spike output file
            }

    if in_argv is not None:
        global_dict = globals()
        local_dict = locals()
        for arg in in_argv:
            try:
                exec(arg, global_dict, local_dict)
            except:
                print("WARNING: Do not know how to process statement \n'{0}'".format(item))
        if 'sim_var' in local_dict:
            # Replace default values with those in the local_dict
            sim_var.update(local_dict['sim_var'])

    return sim_var

def run(argv = None):
    """Run a ring network simulation. Additional arguments on command line can
    be any Python statement to execute, but strings in the form 
    ``"sim_var['<var>']=x"`` will run the simulation with a modification of 
    those parameters."""
    sim_var = process_args(argv)
    pc = h.ParallelContext()
    ring = Ring(sim_var['N'], 
            sim_var['stim_w'], 
            sim_var['stim_spike_num'], 
            sim_var['syn_w'], 
            sim_var['syn_delay'])
    pc.set_maxstep(10)
    h.stdinit()
    h.dt = 0.025 # Fixed dt
    pc.psolve(100)
    ring.write_spikes(sim_var['spike_out_file'])
    
    # After we are done, re-sort the file by spike times.
    exec_cmd = ('sort -k 1n,1n -k 2n,2n ' + sim_var['spike_out_file'] +
            ' > ' + 'sorted_' + sim_var['spike_out_file'])
    os.system(exec_cmd)
        
if __name__ == '__main__':
    run(sys.argv[1:])
