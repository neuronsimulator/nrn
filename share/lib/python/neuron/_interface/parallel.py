"""ParallelContext with Python callables and Python objects.

NEURON's bulletin board carries Python callables and objects as pickles
through hooks only its own Python module fills (methods.po2pickle,
pickle2po, call_picklef). They pass ``std::vector<char>`` by value, which a
ctypes callback cannot implement, and with them unset core calls a null
pointer (pc.post of a list) or asserts (pc.upkpyobj, pc.pyret). So the
pickling happens here instead, on the Python side:

* ``pc.submit(f, *args)`` and ``pc.context(f, *args)`` submit the HOC
  function ``_mn_pyjob`` with one string: the pickled job. Numbers are passed
  as floats, as NEURON passes them through HOC.
* ``_mn_pyjob`` runs ``f(*args)`` and returns the job's token, which
  ``pc.working()`` then leaves in ``pc.retval()``. The value stays in the
  submitting process when that process ran the job; another rank posts it
  under the job's own key, and the submitter takes it in ``pc.working()``.
* ``pc.pyret()`` returns the value of the job ``pc.working()`` just returned;
  ``pc.retval()`` is 0 for such a job, as in NEURON. ``pc.upkscalar()``,
  ``upkstr``, ``upkvec`` and ``upkpyobj`` then return the job's arguments in
  order, from the copy the submitter kept.
* ``pc.post``/``pc.pack`` send each Python object (anything but a number, a
  string or a Vector) as a marked string; ``pc.upkpyobj()`` unpacks it.
* ``pc.py_alltoall``, ``py_allgather``, ``py_gather``, ``py_broadcast`` and
  ``py_scatter`` pickle here and move the bytes with HOC's ``pc.alltoall``
  (NEURON's own implementation reads its arguments from its module's call
  and crashes when called through the C API).
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
# token -> (result key, arguments) of a job this process submitted.
_submitted = {}
# Token of the Python job pc.working() last returned, or None.
_current = [None]
# Arguments of that job not yet unpacked with pc.upk*; None once another
# message (pc.take, pc.look, ...) is the one being unpacked.
_unpack = [None]

# Python-object collectives (pc.py_alltoall and friends), as NEURON's
# nrnpy_p2h.cpp py_alltoall_type: pickled here and exchanged with HOC's
# pc.alltoall on Vectors, one byte per element.
COLLECTIVES = frozenset(
    {"py_alltoall", "py_allgather", "py_gather", "py_broadcast", "py_scatter"}
)
UNPACK = frozenset({"upkscalar", "upkstr", "upkvec", "upkpyobj"})
# Methods that make another message the one pc.upk* reads.
_NEW_MESSAGE = frozenset({"take", "look", "look_take"})
PYTHON_METHODS = (
    frozenset({"submit", "context", "post", "pack", "pyret", "working", "retval"})
    | COLLECTIVES
    | UNPACK
    | _NEW_MESSAGE
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


def _pc():
    from . import n

    return n.ParallelContext()


def run_job(payload):
    """Body of the HOC function ``_mn_pyjob``: run a submitted Python job."""
    token, key, owner, func, args = _decode(payload)
    value = func(*args)
    if key is None:  # pc.context: no result
        return token
    pc = _pc()
    if int(pc.id_world()) == owner:
        _results[token] = value
    elif int(pc.id_bbs()) >= 0:  # -1 on a subworld's other ranks, which run it too
        pc.post(key, _encode(value))
    return token


def _submit(pc, native, name, args):
    i = 1 if args and isinstance(args[0], numbers.Real) else 0
    if len(args) <= i or isinstance(args[i], str) or _is_hoc_object(args[i]):
        return native(*args)
    token = _TOKEN_BASE + next(_next_token)
    owner = int(pc.id_world())
    job_args = tuple(_as_hoc_arg(a) for a in args[i + 1 :])
    key = None
    if name == "submit":
        key = "_mn_ret_%d_%d" % (owner, token - _TOKEN_BASE)
        _submitted[token] = (key, job_args)
    payload = (token, key, owner, args[i], job_args)
    return native(*args[:i], "_mn_pyjob", _encode(payload))


def _start_job(pc, token):
    """pc.working() returned the Python job ``token``: fetch its value."""
    key, job_args = _submitted.pop(token)
    if token not in _results:  # another rank ran it and posted the value
        from . import n

        pc.take(key)
        text = n.ref("")
        pc.upkstr(text)
        _results[token] = _decode(text[0])
    _current[0] = token
    _unpack[0] = list(job_args)


def _next_arg(kind):
    if not _unpack[0]:
        raise RuntimeError("upk: no more items in the message")
    value = _unpack[0].pop(0)
    if kind == "scalar" and isinstance(value, numbers.Real):
        return float(value)
    if kind == "str" and isinstance(value, str):
        return value
    if kind == "vec" and _is_hoc_object(value) and value._class_name == "Vector":
        return value
    if kind == "pyobj":
        return pickle.loads(pickle.dumps(value))  # a copy, as from a pickle
    raise RuntimeError(f"upk{kind}: the next item is a {type(value).__name__}")


def _unpacker(pc, name, native):
    if name == "upkpyobj":

        def upkpyobj():
            if _unpack[0] is not None:
                return _next_arg("pyobj")
            from . import n

            text = n.ref("")
            pc.upkstr(text)
            return _decode(text[0])

        return upkpyobj
    kind = name[3:]

    def upk(*args):
        if _unpack[0] is None:
            return native(*args)
        value = _next_arg(kind)
        if kind == "str":
            args[0][0] = value
        elif kind == "vec":
            if args:
                args[0].resize(0)
                args[0].append(value)
                return args[0]
            return value.c()
        return value

    return upk


def _exchange(pc, chunks):
    """Send chunks[i] (bytes) to rank i; return the bytes each rank sent here."""
    from . import n

    nhost = len(chunks)
    sent = n.Vector([len(c) for c in chunks])
    received = n.Vector()
    pc.alltoall(sent, n.Vector(nhost, 1), received)
    data = n.Vector()
    pc.alltoall(n.Vector(list(b"".join(chunks))), sent, data)
    data = bytes(int(x) for x in data)
    out, start = [], 0
    for count in received:
        out.append(data[start : start + int(count)])
        start += int(count)
    return out


def _loads(data):
    return pickle.loads(data) if data else None


def _check_list(source, name, nhost):
    if not isinstance(source, list):
        raise TypeError("Argument must be a Python list")
    if len(source) != nhost:
        raise ValueError(f"{name} list size must be nhost")


def _collective(pc, name, source, size=0):
    """pc.<name>(source[, size]); size is the root rank, or for py_alltoall
    a negative size returns (bytes sent, bytes received) only."""
    nhost, rank, size = int(pc.nhost()), int(pc.id()), int(size)
    if name == "py_alltoall" or (name == "py_scatter" and rank == size):
        _check_list(source, name, nhost)
    if nhost == 1:  # NEURON returns its argument, repackaged, without pickling
        if name == "py_alltoall":
            return source
        if name == "py_scatter":
            return source[0]
        return source if name == "py_broadcast" else [source]
    if name == "py_alltoall":
        chunks = [b"" if x is None else pickle.dumps(x) for x in source]
        if size < 0:
            from . import n

            received = n.Vector()
            pc.alltoall(
                n.Vector([len(c) for c in chunks]), n.Vector(nhost, 1), received
            )
            return (sum(len(c) for c in chunks), int(sum(received)))
        return [_loads(c) for c in _exchange(pc, chunks)]
    if name == "py_allgather":
        return [_loads(c) for c in _exchange(pc, [pickle.dumps(source)] * nhost)]
    if not 0 <= size < nhost:
        raise RuntimeError("root rank must be >= 0 and < nhost")
    root = size
    if name == "py_gather":
        data = pickle.dumps(source)
        chunks = [data if i == root else b"" for i in range(nhost)]
        received = _exchange(pc, chunks)
        return [_loads(c) for c in received] if rank == root else None
    if name == "py_broadcast":
        data = pickle.dumps(source) if rank == root else b""
        chunks = [data if i != root else b"" for i in range(nhost)]
        received = _exchange(pc, chunks)
        return source if rank == root else _loads(received[root])
    # py_scatter
    if rank == root:
        chunks = [b"" if x is None else pickle.dumps(x) for x in source]
    else:
        chunks = [b""] * nhost
    return _loads(_exchange(pc, chunks)[root])


def python_method(pc, name, native):
    """``pc.<name>`` for a name in PYTHON_METHODS, built on the native method."""
    if name in COLLECTIVES:
        return lambda source, *size: _collective(pc, name, source, *size)
    if name in UNPACK:
        return _unpacker(pc, name, native)
    if name in ("submit", "context"):
        return lambda *args: _submit(pc, native, name, args)
    if name in _NEW_MESSAGE:

        def new_message(*args):
            _unpack[0] = None
            return native(*args)

        return new_message
    if name == "post":
        return lambda key, *args: native(key, *(_packable(a) for a in args))
    if name == "pack":
        return lambda *args: native(*(_packable(a) for a in args))
    if name == "working":

        def working():
            if _current[0] is not None:
                _results.pop(_current[0], None)  # never taken by pyret()
                _current[0] = None
            _unpack[0] = None
            job = native()
            if job:
                value = pc.retval()  # native while _current is None
                if value in _submitted:
                    _start_job(pc, value)
            return job

        return working
    if name == "retval":
        return lambda: 0.0 if _current[0] is not None else native()
    if name == "pyret":

        def pyret():
            token = _current[0]
            if token is None or token not in _results:
                raise RuntimeError("pyret: the last job did not return a Python object")
            return _results.pop(token)

        return pyret
    raise AssertionError(name)
