"""
Import this module if you would like to use the NEURON GUI.

It loads nrngui.hoc, and starts a thread to periodically process
the NEURON GUI event loop.

Note that python threads are not used if nrniv is launched instead of Python
"""

from neuron import h
from contextlib import contextmanager
import threading
import time
import atexit

# recursive, especially in case stop/start pairs called from doNotify code.
# It is the interface's HOC lock: doNotify takes that one too, and with two
# locks a stop() or disabled() from code already holding the HOC lock (a HOC
# callback, rxd's initialization) waited for this one while the timer thread
# held it and waited for the HOC lock.
from neuron._interface.api import HOC_LOCK as _lock


def stop():
    _lock.acquire()


def start():
    _lock.release()


"""
This allows one to temporarily disable the process_events loop using a construct
like:
from neuron import gui
with gui.disabled():
    something_that_would_interact_badly_with_process_events()
"""


@contextmanager
def disabled():
    stop()
    try:
        yield None
    finally:
        start()


def process_events():
    _lock.acquire()
    try:
        h.doNotify()
    except Exception as e:
        print(f"Exception in gui thread: {e}")
    finally:
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
    A Timer that calls a function at regular intervals.
    """

    def __init__(self, interval, fun):
        self.started = False
        self.interval = interval
        self.fun = fun
        self._running = threading.Event()
        threading.Thread.__init__(self, daemon=True)

    def start(self):
        super().start()
        # Not in the new thread: this one may hold the interface's HOC lock (the
        # first import of neuron.gui can be in a HOC call), and waits for it.
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
        # h.quit() runs the exit handlers, and so this, while it holds the
        # interface's HOC lock. If the thread is waiting for that lock to call
        # doNotify it can't finish, and an unbounded join hung the exit. A
        # thread that isn't stuck ends within one interval.
        self.join(timeout=1.0)


if h.nrnversion(9) == "2":  # Launched with Python (instead of nrniv)
    # Callbacks run on this thread while python -i holds the tty in
    # pyrepl/raw mode (OPOST off). Restore NL->CRNL for those prints.
    from neuron import install_raw_tty_newline_fix

    install_raw_tty_newline_fix()
    timer = LoopTimer(0.1, process_events)
    timer.start()

    def cleanup():
        if timer.started:
            timer.stop()

    atexit.register(cleanup)

h.load_file("nrngui.hoc")

h.movie_timer = Timer("moviestep()")
