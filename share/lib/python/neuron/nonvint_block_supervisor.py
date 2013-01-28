import os
import ctypes
import neuron
import numpy
import sys
from neuron import h
import neuron

#
# connect to dll via ctypes
#

nrn_dll = neuron.nrn_dll()

# declare prototype
nonvint_block_prototype = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.POINTER(ctypes.c_double), ctypes.POINTER(ctypes.c_double), ctypes.c_int)

set_nonvint_block = nrn_dll.set_nonvint_block
set_nonvint_block.argtypes = [nonvint_block_prototype]

# Some info not available from the HocObject
v_structure_change = ctypes.c_int.in_dll(nrn_dll, "v_structure_change")

# items in call are each a list of 8 callables
#    [setup, initialize, # method 0, 1
#    fixed_step_setup, fixed_step_solve, # method 2, 3
#    ode_count, ode_reinit, ode_fun, ode_solve ] # method 4-7
call = []

# e.g.
test = '''
def setup(): #0
  print "setup"
def initialize(): #1
  print "initialize"
def current(rhs): #2
  print "outward current to be subtracted from rhs"
def conductance(d): #3
  print "conductance to be added to d"
def fixed_step_solve(dt): #4
  print "fixed_step_solve"
def ode_count(offset): #5
  print "ode_count" ; return 0
def ode_reinit(y): #6 # fill y with initial values of states
  print "ode_reinit"
def ode_fun(t, y, ydot): #7 # from t, y, determine ydot
  print "ode_fun"
def ode_solve(dt, b, y): #8 #solve mx=b replace b with x (y available if m depends on it
  print "ode_solve"
call.append([
    setup, initialize, # method 0, 1
    current, conductance, fixed_step_solve, # method 2, 3, 4
    ode_count, ode_reinit, ode_fun, ode_solve # method 5-8
  ])
'''

ode_count_method_index = 5
arrays_not_used = [0, 1, 4, 5]
arrays_both_used = [7, 8]

def register(c):
  unregister(c)
  call.append(c)

def unregister(c):
  v_structure_change.value = 1
  if c in call:
    call.remove(c)

def ode_count_all(offset):
  global nonvint_block_offset
  nonvint_block_offset = offset
  cnt = 0
  for c in call:
    if c[ode_count_method_index]:
      cnt += c[ode_count_method_index](nonvint_block_offset + cnt)
  #print "ode_count_all %d, offset=%d\n"%(cnt, nonvint_block_offset)
  return cnt

pc = h.ParallelContext()

# see nrn/src/nrnoc/nonvintblock.h for the magic method numbers

def nonvint_block(method, size, pd1, pd2, tid):
  #print 'nonvint_block called with method = %d l=%d' % (method,size)
  assert(tid == 0)
  rval = 0
  try:
    if method == ode_count_method_index:
        rval = ode_count_all(size) # count of the extra states-equations managed by us
    else:
        if pd1:
            pd1_array = numpy.frombuffer(numpy.core.multiarray.int_asbuffer(ctypes.addressof(pd1.contents), size*numpy.dtype(float).itemsize))
        else:
            pd1_array = None
        if pd2:
            pd2_array = numpy.frombuffer(numpy.core.multiarray.int_asbuffer(ctypes.addressof(pd2.contents), size*numpy.dtype(float).itemsize))
        else:
            pd2_array = None
        
        if method in [0, 1]:
            args = ()
        elif method in [2, 3, 6]:
            args = (pd1_array,)
        elif method == 4:
            args = (pc.dt(tid),)
        elif method == 7:
            args = (pc.t(tid), pd1_array, pd2_array)
        elif method == 8:
            args = (pc.dt(tid), pd1_array, pd2_array)
        for c in call:
            if c[method] is not None:
                c[method](*args)
  except:
    print sys.exc_info()[0], ': ', sys.exc_info()[1]
    rval = -1
  return rval

_callback = nonvint_block_prototype(nonvint_block)
set_nonvint_block(_callback)

if __name__ == '__main__':
  exec(test) # see above string

  s = h.Section()
  print "fixed step finitialize"
  h.finitialize(0)
  print "fixed step fadvance"
  h.fadvance()

  h.load_file('stdgui.hoc')
  print "cvode active"
  h.cvode_active(1)
  print "cvode step finitialize"
  h.finitialize(0)
  print "cvode fadvance"
  h.fadvance()

