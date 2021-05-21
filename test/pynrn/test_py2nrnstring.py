from neuron import h
from neuron.expect_hocerr import expect_hocerr, quiet, set_quiet, printerr, expect_err
import sys

uni = 'ab\xe0'

def checking(s):
  if not quiet:
    print("CHECKING: " + s)

class Foo: # for testing section name errors when full cell.section name has unicode chars.
  def __init__(self, name):
    self.name = name
  def __str__(self):
    return self.name

def test_py2nrnstring():
  print (uni)

  '''
  # I don't think h.getstr() can be made to work from python
  # so can't test unicode on stdin read by hoc.
  h('strdef s')
  h('s = "hello"')
  checking ("h('getstr(s)')")
  h('getstr(s)')
  'goodbye'
  assert h.s == 'hello'

  checking('h(uni)')
  h(uni)
  '''

  checking('h.printf("%s", uni)')
  expect_hocerr(h.printf, ("%s", uni))

  checking('hasattr(h, uni)')
  expect_hocerr(hasattr, (h, uni))

  expect_err('h.à = 1', globals(), locals())

  expect_err('a = h.à', globals(), locals())

  expect_err('a = h.ref("à")', globals(), locals())
  err = 0
  try:
    a = h.ref("à")
  except Exception as e:
    printerr(e)
    err = 1
  assert err == 1

  ns = h.NetStim()
  #nonsense but it does test the error unicode error message
  checking('h.setpointer(ns._ref_start, "à", ns)')
  expect_hocerr(h.setpointer, (ns._ref_start, "à", ns))
  del ns

  '''
  # No error for these (unless cell is involved)!
  expect_err('h.Section(name=uni)', globals(), locals())

  expect_err('h.Section(uni)', globals(), locals())
  '''
  
  expect_err('h.Section(name="apical", cell=Foo(uni))', globals(), locals())

  soma = h.Section()
  expect_err('a = soma.à', globals(), locals())

  expect_err('soma.à = 1', globals(), locals())

  expect_err('a = soma(.5).à', globals(), locals())

  expect_err('soma(.5).à = 1', globals(), locals())

  soma.insert("hh")
  expect_err('a = soma(.5).hh.à', globals(), locals())

  expect_err('soma(.5).hh.à = 1', globals(), locals())

if __name__ == '__main__':
  set_quiet(False)
  quiet = False
  test_py2nrnstring()

