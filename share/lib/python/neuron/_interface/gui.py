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
from .api import HOC_LOCK

# HOC_LOCK, the lock every HOC call holds, so doNotify never runs inside
# another thread's HOC call and stop()/start() order with HOC calls under one
# lock (two locks could deadlock: a callback calling stop() inside a HOC call
# while the event thread holds the loop lock and waits for HOC). Reentrant:
# stop()/start() may be called from doNotify callbacks.
_lock = HOC_LOCK


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
        # Sets the interval and returns it, as HOC's Timer.seconds(x) and
        # neuron.gui.Timer do.
        self._interval = interval
        return interval

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

    def start(self):
        super().start()
        # Not in the new thread: this one may hold HOC_LOCK (the first import of
        # the module can be in a HOC call), and waits for it.
        h.nrniv_bind_thread(self.ident)
        self.started = True
        self._running.set()

    def run(self):
        self._running.wait()
        while self._running.is_set():
            self.fun()
            time.sleep(self.interval)

    def stop(self):
        """Stop the timer thread and wait (a bounded time) for it to end."""
        self._running.clear()
        # h.quit() runs the exit handlers, and so this, holding HOC_LOCK, for
        # which the thread may be waiting.
        self.join(timeout=1.0)


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

h.load_file("nrngui.hoc")

# movie_timer mirrors NEURON's GUI movie-recording infrastructure.
# moviestep() is a HOC procedure defined in nrngui.hoc.
h.movie_timer = Timer("moviestep()")
