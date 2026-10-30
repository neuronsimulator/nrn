"""Lazy HOC documentation data, without importing NEURON's Python binding."""
import ast
import functools
import importlib.util
import io
import pickle
import zlib
from pathlib import Path

_HELP = None
_BUNDLED_HELP = Path(__file__).with_name("_data") / "help_data.dat"


class _HelpUnpickler(pickle.Unpickler):
    def find_class(self, module, name):
        # The upstream format is a compressed dict[str, str], not executable
        # objects. Refuse GLOBAL/REDUCE constructors even from installed data.
        raise pickle.UnpicklingError("documentation cannot load Python globals")


def _help_paths():
    try:
        spec = importlib.util.find_spec("neuron")
    except (ImportError, ValueError):
        spec = None
    if spec is not None:
        for directory in spec.submodule_search_locations or ():
            yield Path(directory) / "help_data.dat"
    yield _BUNDLED_HELP


def _load_help():
    global _HELP
    # Concurrent first reads may duplicate this read-only load; publishing a
    # fully validated dictionary is atomic, so that benign race is tolerated.
    if _HELP is None:
        for path in _help_paths():
            try:
                raw = zlib.decompress(path.read_bytes())
                data = _HelpUnpickler(io.BytesIO(raw)).load()
                if type(data) is not dict or not all(
                    type(key) is str and type(value) is str
                    for key, value in data.items()
                ):
                    continue
            except Exception:
                continue
            _HELP = data
            break
        else:
            # Help must not disable otherwise working HOC dispatch when a
            # source checkout or third-party package omits documentation data.
            _HELP = {}
    return _HELP


def get_doc(key, fallback=""):
    """Return the installed core's help text, or the bundled/fallback text."""
    return _load_help().get(key, fallback)


_TOP_LEVEL_DOC = None


def top_level_doc():
    """neuron.doc's ``doc_h``, the text of ``h.__doc__``.

    Read from the neuron package's doc.py without importing the package
    (importing neuron would load its compiled extension); empty without it.
    """
    global _TOP_LEVEL_DOC
    if _TOP_LEVEL_DOC is None:
        _TOP_LEVEL_DOC = ""
        try:
            spec = importlib.util.find_spec("neuron")
            for directory in (spec.submodule_search_locations or ()) if spec else ():
                path = Path(directory) / "doc.py"
                if not path.is_file():
                    continue
                for node in ast.parse(path.read_text(encoding="utf-8")).body:
                    if (
                        isinstance(node, ast.Assign)
                        and [getattr(t, "id", None) for t in node.targets] == ["doc_h"]
                        and isinstance(node.value, ast.Constant)
                        and isinstance(node.value.value, str)
                    ):
                        _TOP_LEVEL_DOC = node.value.value
                break
        except Exception:
            pass
    return _TOP_LEVEL_DOC


class _HelpDocMethod:
    """A Python-defined method whose ``__doc__`` is NEURON's help text for
    ``<class>.<name>`` (``Vector.to_python``), loaded on first ``__doc__``."""

    def __init__(self, func, key):
        # __doc__ is the property below, so update_wrapper cannot be used.
        functools.update_wrapper(
            self, func, assigned=("__module__", "__name__", "__qualname__")
        )
        self._func = func
        self._key = key

    def __get__(self, instance, owner=None):
        if instance is None:
            return self
        return _BoundHelpDocMethod(self._func, instance, self._key)

    def __call__(self, *args, **kwargs):
        return self._func(*args, **kwargs)

    @property
    def __doc__(self):
        return get_doc(self._key, self._func.__doc__ or "")


class _BoundHelpDocMethod:
    __slots__ = ("__func__", "__self__", "_key")

    def __init__(self, func, instance, key):
        self.__func__ = func
        self.__self__ = instance
        self._key = key

    def __call__(self, *args, **kwargs):
        return self.__func__(self.__self__, *args, **kwargs)

    @property
    def __doc__(self):
        return get_doc(self._key, self.__func__.__doc__ or "")

    @property
    def __name__(self):
        return self.__func__.__name__

    def __repr__(self):
        return f"<bound method {self.__func__.__qualname__} of {self.__self__!r}>"


def help_doc(key):
    """Decorator: the method's __doc__ is NEURON's help text for `key`."""
    return lambda func: _HelpDocMethod(func, key)


# neuron.doc's texts for a class or object without help data.
_CLASS_DOC_TEMPLATE = """
No docstring available for class '%s'

Try checking the online documentation at:
https://www.neuronsimulator.org/
"""
_OBJECT_DOC_TEMPLATE = """
No docstring available for object type '%s'

Try checking the online documentation at:
https://www.neuronsimulator.org/
"""


def _class_help(name, owner):
    """neuron.doc's text for a class: its help, then each member's."""
    help_data = _load_help()
    text = help_data.get(name, "")
    if not text:
        return ""
    for member in dir(owner):
        member_text = help_data.get(f"{name}.{member}")
        if member_text:
            text += f"\n\n\n{name}.{member}:\n\n{member_text}"
    return text


class _ClassDoc:
    """Non-data descriptor: class creation and ordinary use do no help I/O.

    ``h.Vector.__doc__`` and ``h.Vector().__doc__`` are neuron.doc's: the
    class's help followed by its members', or the "No docstring available"
    text for a class or an object. A docstring given at class creation wins.
    """

    def __init__(self, name, fallback=None):
        self.name = name
        self.fallback = fallback

    def __get__(self, instance, owner=None):
        if self.fallback:
            return get_doc(self.name, self.fallback)
        text = _class_help(self.name, owner)
        if text:
            return text
        template = _CLASS_DOC_TEMPLATE if instance is None else _OBJECT_DOC_TEMPLATE
        return template % self.name
