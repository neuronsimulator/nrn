"""The GUI timer thread and HOC calls on the main thread must not deadlock.

neuron.gui's timer thread calls doNotify every 0.1 s, which needs the
interface's HOC lock. Each case runs in a subprocess (a deadlock times out
there instead of hanging pytest).
"""

import os
import subprocess
import sys
import textwrap

QUIT = textwrap.dedent(
    """
    # h.quit() holds the HOC lock while NEURON runs its exit handlers, and
    # neuron.gui's cleanup joins the timer thread. If that thread was waiting
    # for the lock the join never returned (coreneuron spikes tests hung).
    import time
    from neuron import h, gui
    from neuron._interface.api import HOC_LOCK

    with HOC_LOCK:
        time.sleep(0.5)  # the timer thread blocks on the lock
        h.quit()
    """
)

STOP = textwrap.dedent(
    """
    # gui.stop() from code that holds the HOC lock (a HOC callback, rxd's
    # initialization). With a separate gui lock, the timer thread held it and
    # waited for the HOC lock while this waited for the gui lock.
    import time
    from neuron import h, gui
    from neuron._interface.api import HOC_LOCK

    with HOC_LOCK:
        time.sleep(0.5)
        gui.stop()
        gui.start()
    print("done")
    """
)

# The first import of neuron.gui, which starts the timer thread, in a HOC call.
IMPORT = """from neuron import h; h('nrnpython("import neuron.gui")'); print("done")"""


def run(code, options="-nogui"):
    env = dict(os.environ, NEURON_MODULE_OPTIONS=options)
    # macOS does not pass the sanitizer preload on to child processes (as in
    # test_basic.py's test_errorcode).
    if "NRN_SANITIZER_PRELOAD_VAR" in os.environ:
        env[os.environ["NRN_SANITIZER_PRELOAD_VAR"]] = os.environ[
            "NRN_SANITIZER_PRELOAD_VAL"
        ]
    return subprocess.run(
        [os.environ.get("NRN_PYTHON_EXECUTABLE", sys.executable), "-c", code],
        env=env,
        capture_output=True,
        text=True,
        timeout=60,
    )


def test_quit_while_gui_timer_waits_for_hoc_lock():
    result = run(QUIT)
    assert result.returncode == 0, result.stdout + result.stderr


def test_stop_and_import_while_holding_hoc_lock():
    for code in STOP, IMPORT:
        result = run(code)
        assert "done" in result.stdout and not result.returncode, result.stderr


if sys.platform == "win32":  # the timer thread makes the window, and calls HOC

    def test_window_from_the_main_thread():
        result = run("from neuron import h, gui; h.Graph(); h.quit()", "")
        assert result.returncode == 0, result.stdout + result.stderr
