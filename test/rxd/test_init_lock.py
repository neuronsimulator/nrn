"""rxd's init lock and the interface's HOC lock must be one lock.

`from neuron import gui` starts a thread that calls doNotify, and NEURON can
call rxd's _setup_matrices from inside it, so that thread holds the HOC lock
and then wants rxd's lock. The main thread takes rxd's lock in
Species.__init__ and then makes HOC calls. With two locks, those orders
deadlocked (rxd-tutorials/basic-initialization.ipynb hung the docs build).
The interleaving runs in a subprocess, so a regression fails instead of
hanging the rest of the rxd tests.
"""

import os
import subprocess
import sys
import textwrap

CHILD = textwrap.dedent(
    """
    import sys, threading
    from neuron import h
    from neuron.rxd import initializer

    entered_hoc = threading.Event()
    done = []

    def callback():
        # Runs inside a HOC call on the other thread, like _setup_matrices
        # inside doNotify.
        entered_hoc.set()
        with initializer._init_lock:
            done.append("callback")

    sys.modules["__main__"].callback = callback

    def hoc_then_rxd_lock():
        h.nrnpython("import __main__; __main__.callback()")
        done.append("hoc thread")

    with initializer._init_lock:
        other = threading.Thread(target=hoc_then_rxd_lock, daemon=True)
        other.start()
        # With one lock the other thread can't enter HOC yet; with two it
        # enters, sets the event and waits for this lock.
        entered_hoc.wait(timeout=2)
        h("objref rxd_lock_test")
        done.append("rxd lock holder")
    other.join(timeout=20)
    print(sorted(done))
    """
)


def test_init_lock_is_the_hoc_lock(neuron_instance):
    from neuron import _interface
    from neuron.rxd import initializer

    assert initializer._init_lock is _interface.api.HOC_LOCK


def test_hoc_callback_waiting_for_rxd_lock():
    result = subprocess.run(
        [sys.executable, "-c", CHILD],
        env=dict(os.environ, NEURON_MODULE_OPTIONS="-nogui"),
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    assert "['callback', 'hoc thread', 'rxd lock holder']" in result.stdout, (
        result.stdout + result.stderr
    )
