"""ParallelContext with Python callables and Python objects, for one process.

NEURON's bulletin board carries Python callables and objects as pickles
through hooks only its own Python module fills (methods.po2pickle,
pickle2po, call_picklef). They pass ``std::vector<char>`` by value, which a
ctypes callback cannot implement, and with them unset core calls a null
pointer (pc.post of a list) or asserts (pc.upkpyobj, pc.pyret). So the
pickling happens here instead, on the Python side:

* ``pc.submit(f, *args)`` and ``pc.context(f, *args)`` submit the HOC
  function ``_mn_pyjob`` with one string: the pickled ``(token, f, args)``.
  Numbers are passed as floats, as NEURON passes them through HOC.
* ``_mn_pyjob`` runs ``f(*args)``, keeps the return value under ``token`` and
  returns the token, which ``pc.working()`` then leaves in ``pc.retval()``.
* ``pc.pyret()`` returns the value kept for the job ``pc.working()`` just
  returned; ``pc.retval()`` is 0 for such a job, as in NEURON.
* ``pc.post``/``pc.pack`` send each Python object (anything but a number, a
  string or a Vector) as a marked string; ``pc.upkpyobj()`` unpacks it.

The job runs in the process that calls ``pc.working()``, which is where the
kept value lives, so this needs a single process (nhost 1); with more,
submitting a Python callable raises (gap-73). After ``pc.working()`` returns
a Python job, ``pc.upkscalar()`` and friends see the encoded string, not the
submitted arguments.
"""

import base64
import itertools
import numbers
import pickle

# Marks a string that carries a pickled Python object.
_PICKLE_MARK = "\x01myneuron-pickle:"

# Tokens are far above any job id or ordinary return value.
_TOKEN_BASE = 2.0**52
_next_token = itertools.count(1)

# token -> return value of a finished Python job not yet taken by pyret().
_results = {}
# Token of the Python job pc.working() last returned, or None.
_current = [None]

PYTHON_METHODS = frozenset(
    {"submit", "context", "post", "pack", "upkpyobj", "pyret", "working", "retval"}
)


def _encode(obj):
    return _PICKLE_MARK + base64.b64encode(pickle.dumps(obj)).decode("ascii")


def _decode(text):
    if not isinstance(text, str) or not text.startswith(_PICKLE_MARK):
        raise RuntimeError("upkpyobj: the next item is not a Python object")
    return pickle.loads(base64.b64decode(text[len(_PICKLE_MARK) :]))


def _is_hoc_object(value):
    from .object import Object

    return isinstance(value, Object)


def _as_hoc_arg(value):
    """``value`` as NEURON passes a submit argument to a Python job."""
    if isinstance(value, numbers.Real):
        return float(value)
    return value


def _packable(value):
    """``value`` as a post/pack item: Python objects become marked strings."""
    if isinstance(value, (numbers.Real, str)):
        return value
    if _is_hoc_object(value) and value._class_name == "Vector":
        return value
    return _encode(value)


def run_job(payload):
    """Body of the HOC function ``_mn_pyjob``: run a submitted Python job."""
    token, func, args = _decode(payload)
    _results[token] = func(*args)
    return token


def _submit(pc, native, args):
    i = 1 if args and isinstance(args[0], numbers.Real) else 0
    if len(args) <= i or isinstance(args[i], str) or _is_hoc_object(args[i]):
        return native(*args)
    if pc.nhost() > 1:
        raise NotImplementedError(
            "ParallelContext: a Python callable job needs a single process "
            "in myneuron (gap-73)"
        )
    token = _TOKEN_BASE + next(_next_token)
    payload = (token, args[i], tuple(_as_hoc_arg(a) for a in args[i + 1 :]))
    return native(*args[:i], "_mn_pyjob", _encode(payload))


def python_method(pc, name, native):
    """``pc.<name>`` for a name in PYTHON_METHODS, built on the native method."""
    if name in ("submit", "context"):
        return lambda *args: _submit(pc, native, args)
    if name == "post":
        return lambda key, *args: native(key, *(_packable(a) for a in args))
    if name == "pack":
        return lambda *args: native(*(_packable(a) for a in args))
    if name == "upkpyobj":

        def upkpyobj():
            from . import n

            text = n.ref("")
            pc.upkstr(text)
            return _decode(text[0])

        return upkpyobj
    if name == "working":

        def working():
            if _current[0] is not None:
                _results.pop(_current[0], None)  # never taken by pyret()
                _current[0] = None
            job = native()
            if job:
                value = pc.retval()  # native while _current is None
                if value in _results:
                    _current[0] = value
            return job

        return working
    if name == "retval":
        return lambda: 0.0 if _current[0] is not None else native()
    if name == "pyret":

        def pyret():
            token = _current[0]
            if token is None or token not in _results:
                raise RuntimeError("pyret: the last job did not return a Python object")
            _current[0] = None
            return _results.pop(token)

        return pyret
    raise AssertionError(name)
