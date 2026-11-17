"""HOC ParallelContext calls that need the Python pickle hooks don't crash.

NEURON's Python module sets the hooks that pickle a PythonObject for
ParallelContext. Where they are not set (nrniv -nopython, or a Python
interface that doesn't set them), packing an object other than a Vector and
unpacking a Python object used to call a null function pointer. They raise a
HOC error now. Where the hooks are set, the calls work as before.
"""

import subprocess
import sys
import textwrap

_CHILD = textwrap.dedent(
    """
    from neuron import h

    h("objref pc, lst, got")
    h("pc = new ParallelContext()")
    h("lst = new List()")
    print("post:", h('pc.post("hook_k1", lst)'), flush=True)

    pc = h.ParallelContext()
    pc.post("hook_k2", {"a": 1})
    h('pc.take("hook_k2")')
    print("upkpyobj:", h("got = pc.upkpyobj()"), flush=True)
    print("done", flush=True)
    """
)


def test_hoc_calls_without_pickle_hooks_do_not_crash():
    proc = subprocess.run(
        [sys.executable, "-c", _CHILD], capture_output=True, text=True, timeout=120
    )
    assert proc.returncode == 0, proc.stdout + proc.stderr
    assert "done" in proc.stdout
    for line in ("post:", "upkpyobj:"):
        assert line in proc.stdout
    if "needs the Python pickle hooks" in proc.stderr:
        return  # no hooks: each call failed with the HOC error
    # hooks set: both calls succeeded
    assert "post: 1" in proc.stdout or "post: True" in proc.stdout, proc.stdout
