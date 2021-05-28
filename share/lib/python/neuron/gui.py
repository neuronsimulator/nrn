"""
Import this module if you would like to use the NEURON GUI.

It loads nrngui.hoc, and starts a thread to periodically process
the NEURON GUI event loop.
"""


from neuron import h

import threading
import time

# recursive, especially in case stop/start pairs called from doNotify code.
_lock = threading.RLock()


def stop():
    _lock.acquire()


def start():
    _lock.release()


def process_events():
    _lock.acquire()
    try:
        h.doNotify()
    except:
        print("Exception in gui thread")
    _lock.release()


class Timer:
    """Python version of the NEURON Timer class"""

    def __init__(self, callback):
        self._callable = callback
        self._interval = 1
        self._thread = None

    def seconds(self, interval):
        last_interval = interval
        self._interval = interval
        return last_interval

    def _do_callback(self):
        if callable(self._callable):
            self._callable()
        else:
            h(self._callable)
        if self._thread is not None:
            self.start()

    def start(self):
        self._thread = threading.Timer(self._interval, self._do_callback)
        self._thread.start()

    def end(self):
        if self._thread is not None:
            self._thread.cancel()
            self._thread = None


class LoopTimer(threading.Thread):
    """
    a Timer that calls f every interval
    """

    def __init__(self, interval, fun):
        """
        @param interval: time in seconds between call to fun()
        @param fun: the function to call on timer update
        """
        self.started = False
        self.interval = interval
        self.fun = fun
        threading.Thread.__init__(self)
        self.setDaemon(True)

    def run(self):
        h.nrniv_bind_thread(threading.current_thread().ident)
        self.started = True
        while True:
            self.fun()
            time.sleep(self.interval)


if (
    h.nrnversion(9) == "2"
    or h.nrnversion(8).find("mingw") > 0
    or h.nrnversion(8).find("Windows")
):
    timer = LoopTimer(0.1, process_events)
    timer.start()
    while not timer.started:
        time.sleep(0.001)

h.load_file("nrngui.hoc")

h.movie_timer = Timer("moviestep()")
