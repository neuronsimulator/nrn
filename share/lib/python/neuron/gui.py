"""
Import this module if you would like to use the NEURON GUI.

It loads nrngui.hoc, and starts a thread to periodically process
the NEURON GUI event loop.
"""


from neuron import h

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
    self.started = False
    self.interval = interval
    self.fun = fun
    threading.Thread.__init__(self)
    self.setDaemon(True)

  def run(self) :
    h.nrniv_bind_thread(threading.current_thread().ident);
    self.started = True;
    while True:
      self.fun()
      time.sleep(self.interval)

if h.nrnversion(9) == '2' or h.nrnversion(8).find('mingw') > 0:
  timer = LoopTimer(0.1, process_events)
  timer.start()
  while not timer.started:
    time.sleep(0.001)

h.load_file("nrngui.hoc")

