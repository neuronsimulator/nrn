"""
Import this module if you would like to use the NEURON GUI.

Importing this module loads nrngui.hoc and installs the movie timer. When
launched with Python (h.nrnversion(9) == "2"), it also starts a daemon thread
to process GUI events. The nrniv launch path does not start that thread.

Adapted from NEURON's share/lib/python/neuron/gui.py — imports myneuron's
top-level singleton instead of the neuron CPython extension.
"""

import atexit
import threading
import time
from contextlib import contextmanager

from . import n as h

# RLock: stop()/start() may be called re-entrantly from doNotify callbacks.
_lock = threading.RLock()


def stop():
    """Acquire the event-loop lock, blocking process_events until start() is called."""
    _lock.acquire()


def start():
    """Release the event-loop lock, allowing process_events to run again."""
    _lock.release()


@contextmanager
def disabled():
    """Temporarily suspend the process_events loop.

    Use when a block of code would interact badly with concurrent doNotify calls::

        from neuron import gui
        with gui.disabled():
            something_that_would_interact_badly_with_process_events()
    """
    stop()
    try:
        yield None
    finally:
        start()


def process_events():
    """Call h.doNotify() under the lock; print and suppress callback exceptions."""
    # TODO(gap-46): replace print() with logging.getLogger(__name__).warning() so callers
    # can suppress GUI-thread noise via the logging hierarchy.
    _lock.acquire()
    try:
        h.doNotify()
    except Exception as e:
        # Swallow: a GUI callback error must not crash the timer thread.
        print(f"Exception in gui thread: {e}")
    finally:
        _lock.release()


class Timer:
    """Python version of the NEURON Timer class"""

    __slots__ = ("_callable", "_interval", "_thread")

    def __init__(self, callback):
        self._callable = callback
        self._interval = 1
        self._thread = None

    def seconds(self, interval):
        # HOC Timer.seconds(x) sets the interval and returns the *previous* one.
        # Deliberate difference from pinned neuron.gui.Timer (returns the new
        # interval); preserve this contract when syncing upstream GUI code.
        last_interval = self._interval
        self._interval = interval
        return last_interval

    def _do_callback(self):
        # Re-arms after each fire (one-shot threading.Timer, re-created each interval).
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
    A Timer that calls a function at regular intervals.

    No ``__slots__``: ``threading.Thread`` supplies an instance ``__dict__``
    that subclass slots cannot remove. This is not a HOC wrapper class.
    """

    def __init__(self, interval, fun):
        self.started = False
        self.interval = interval
        self.fun = fun
        self._running = threading.Event()
        threading.Thread.__init__(self, daemon=True)

    def run(self):
        # Bind this OS thread to NEURON's thread-local state before processing
        # events. See nrn/src/nrniv/nrniv.cpp:nrniv_bind_thread. This does not
        # make arbitrary concurrent HOC calls safe.
        h.nrniv_bind_thread(threading.current_thread().ident)
        self.started = True
        self._running.set()
        while self._running.is_set():
            self.fun()
            time.sleep(self.interval)

    def stop(self):
        """Stop the timer thread and wait for it to terminate."""
        self._running.clear()
        self.join()


# --- Module import side effects ---
# Everything below runs at import time. Do not import this module from tests
# or non-GUI code paths.
if h.nrnversion(9) == "2":  # Launched with Python (instead of nrniv)
    timer = LoopTimer(0.1, process_events)
    timer.start()

    def cleanup():
        if timer.started:
            timer.stop()

    atexit.register(cleanup)

    # TODO(gap-46): replace spin-wait with threading.Event to avoid hang if LoopTimer.run
    # raises before setting self.started = True.
    while not timer.started:
        time.sleep(0.001)

h.load_file("nrngui.hoc")

# movie_timer mirrors NEURON's GUI movie-recording infrastructure.
# moviestep() is a HOC procedure defined in nrngui.hoc.
h.movie_timer = Timer("moviestep()")
