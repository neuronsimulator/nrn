from neuron import h
from neuron.expect_hocerr import expect_hocerr, quiet, set_quiet, printerr
import sys

uni = 'ab\xe0'

def checking(s):
  if not quiet:
    print("CHECKING: " + s)

def test_py2nrnstring():
  print (uni)

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

  checking('h.printf("%s", uni)')
  expect_hocerr(h.printf, ("%s", uni))

  checking('hasattr(h, uni)')
  expect_hocerr(hasattr, (h, uni))

  checking('h.à = 1')
  err = 0
  try:
    h.à = 1
  except Exception as e:
    printerr(e)
    err = 1
  assert err == 1

  checking('a = h.à')
  err = 0
  try:
    a = h.à
  except Exception as e:
    printerr(e)
    err = 1
  assert err == 1

  checking('a = h.ref("à")')
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
  checking('h.Section(name=uni)')
  err = 0
  try:
    h.Section(name=uni)
  except Exception as e:
    printerr(e)
    err = 1
  assert err == 1

  checking('h.Section(uni)')
  expect_hocerr(h.Section, (uni,))
  '''

  checking('a = soma.à')
  soma = h.Section()
  err = 0
  try:
    a = soma.à
  except Exception as e:
    printerr(e)
    err = 1
  assert err == 1

  checking('soma.à = 1')
  err = 0
  try:
    soma.à = 1
  except Exception as e:
    printerr(e)
    err = 1
  assert err == 1

  checking('a = soma(.5).à')
  err = 0
  try:
    a = soma(.5).à
  except Exception as e:
    printerr(e)
    err = 1
  assert err == 1

  checking('soma(.5).à = 1')
  err = 0
  try:
    soma(.5).à = 1
  except Exception as e:
    printerr(e)
    err = 1
  assert err == 1

  checking('a = soma(.5).hh.à')
  soma.insert("hh")
  err = 0
  try:
    a = soma(.5).hh.à
  except Exception as e:
    printerr(e)
    err = 1
  assert err == 1

  checking('soma(.5).hh.à = 1')
  err = 0
  try:
    soma(.5).hh.à = 1
  except Exception as e:
    printerr(e)
    err = 1
  assert err == 1

if __name__ == '__main__':
  set_quiet(False)
  quiet = False
  test_py2nrnstring()

