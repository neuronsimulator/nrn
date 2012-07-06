"""
Import this module if you would like to use the NEURON GUI.

It loads nrngui.hoc, and starts a thread to periodically process
the NEURON GUI event loop.
"""


from neuron import h
h('load_file("nrngui.hoc")')
#h('load_file("hh.ses")')

import threading
import time
def process_events() :
  #h.doEvents()
  h.doNotify()
  #print "timer"

class LoopTimer(threading.Thread) :
  """
  a Timer that calls f every interval
  """
  def __init__(self, interval, fun) :
    """
    @param interval: time in seconds between call to fun()
    @param fun: the function to call on timer update
    """
    self.interval = interval
    self.fun = fun
    threading.Thread.__init__(self)
    self.setDaemon(True)

  def run(self) :
    while True:
      self.fun()
      time.sleep(self.interval)

if (h.nrnversion(9) == '2'):
  timer = LoopTimer(.2, process_events)
  timer.start()
