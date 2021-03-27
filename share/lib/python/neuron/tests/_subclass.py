from neuron import *

h('''
begintemplate A
public x, s, o, xa, oa, f, p
strdef s
objref o, oa[2]
double xa[3]
proc init() { \
  x = $1 \
}
func f() { return $1*xa[$2] }
proc p() { x += 1 }
endtemplate A
''')

class A1(hclass(h.A)) :
  def __new__(cls, arg):
    return super().__new__(cls, arg)
  
  def __init__(self, arg) : # note, arg used by h.A
    #self.bp = hoc.HocObject.baseattr(self, 'p')
    self.bp = self.baseattr('p')
  def p(self) :
    self.bp()
    return self.x
