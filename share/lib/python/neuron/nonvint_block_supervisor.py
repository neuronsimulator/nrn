import os
import ctypes
import neuron
import numpy
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

# items in call are each a list of 8 callables
#    [setup, initialize, # method 0, 1
#    fixed_step_setup, fixed_step_solve, # method 2, 3
#    ode_count, ode_reinit, ode_fun, ode_solve ] # method 4-7
call = []

# e.g.
test = '''
def setup(unused1, unused2):
  print "setup"
def initialize(unused1, unused2):
  print "initialize"
def fixed_step_setup(d, rhs):
  print "fixed_step_setup"
def fixed_step_solve(unused1, unused2):
  print "fixed_step_solve"
def ode_count(offset):
  print "ode_count" ; return 0
def ode_reinit(y, unused):
  print "ode_reinit"
def ode_fun(y, ydot): # from y determine ydot
  print "ode_fun"
def ode_solve(b, y): #solve mx=b replace b with x (y available if m depends on it
  print "ode_solve"
call.append([
    setup, initialize, # method 0, 1
    fixed_step_setup, fixed_step_solve, # method 2, 3
    ode_count, ode_reinit, ode_fun, ode_solve # method 4-7
  ])
'''

def register(c):
  unregister(c)
  call.append(c)

def unregister(c):
  if c in call:
    call.remove(c)

def ode_count_all(offset):
  global nonvint_block_offset
  nonvint_block_offset = offset
  ode_count_method_index = 4
  cnt = 0
  for c in call:
    if c[ode_count_method_index]:
      cnt += c[ode_count_method_index](nonvint_block_offset + cnt)
  #print "ode_count_all %d, offset=%d\n"%(cnt, nonvint_block_offset)
  return cnt

# see nrn/src/nrnoc/nonvintblock.h for the magic method numbers

def nonvint_block(method, array_length, pd1, pd2, tid):
    print 'nonvint_block called with method = %d l=%d' % (method,array_length)
    assert(tid == 0)
    if method >= 10: # nrn_nonvint_block_ode_count
        # method - 10 is the offset for elements in pd1,pd2 managed by us.	
        return ode_count_all(method - 10) # count of the extra states-equations managed by us
    else:
        try:
            pd1_array = numpy.frombuffer(numpy.core.multiarray.int_asbuffer(ctypes.addressof(pd1.contents), array_length*numpy.dtype(float).itemsize))
        except:
            pd1_array = None

        try:    
            pd2_array = numpy.frombuffer(numpy.core.multiarray.int_asbuffer(ctypes.addressof(pd2.contents), array_length*numpy.dtype(float).itemsize))
        except:
            pd2_array = None
        
        for c in call:
            if c[method] is not None:
                c[method](pd1_array, pd2_array)
    return 0

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

