import __main__
import sys
import traceback
def e(stmt) :
  try:
    print('\n\n'+stmt)
    exec(stmt, __main__.__dict__)
  except:
    #print((str(sys.exc_info()[0]) + ': ' + str(sys.exc_info()[1])))
    traceback.print_exc()


from neuron import h

def test_py2nrnstring():
  uni = 'ab\xe0'
  print (uni)

  if False:
    try:
      h.printf("%s", uni)
    except:
      pass

  e("hasattr(h, '%s')"%uni)

if __name__ == '__main__':
  test_py2nrnstring()

