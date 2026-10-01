"""Reference types for NEURON's pointer-based variable tracking (_ref_ syntax).

Two families live here:

* **Binding refs** (`NrnRangeVarRef`, `NrnVarRef`, `NrnObjectPropertyRef`) — a
  pointer to an *existing* symbol / property / range var, obtained via the
  ``_ref_`` syntax (``seg._ref_v``, ``n._ref_t``, ``ic._ref_amp``).
* **Fresh-value refs** (`NrnDoubleRef`, `NrnStrRef`, `NrnObjectRef`) — a
  brand-new anonymous cell holding a copy of a value, matching real NEURON's
  ``h.ref(x)``: ``h.ref(3.0)`` is a settable double cell, ``h.ref('')`` a
  settable string cell, ``h.ref(obj)`` / ``h.ref(None)`` a settable objref
  cell. These are minted by ``NEURON.ref`` (gap-37). The string cell is the
  tricky one: HOC's ``$s1 = "..."`` writeback (``hoc_assign_str``) does
  ``free(*cpp)`` then ``*cpp = emalloc(...)`` (= ``std::malloc``), so the cell's
  ``char*`` must be libc-``malloc``/``strdup``-allocated or NULL — Python-owned
  ctypes string memory would corrupt the heap on writeback. We therefore back
  ``NrnStrRef`` with a libc-``strdup`` cell. Verified against the NEURON source
  (``nrnpy_hoc.cpp`` ``mkref`` / ``hoc_assign_str``) and empirically through a
  50× HOC-writeback stress loop. Closes the netpyne ``importCell`` /
  ``mechVarList`` blocker.
"""
import abc
import ctypes
import sys
import weakref

# libc handle for the string-ref cell. NrnStrRef's char* must share NEURON's
# allocator (std::malloc, via emalloc) so HOC's hoc_assign_str writeback can
# free()/realloc it. Loaded lazily so importing this module never fails on an
# exotic platform that lacks the default name.
_libc = None


def _get_libc():
    global _libc
    if _libc is None:
        import ctypes.util

        name = ctypes.util.find_library("c") or "libc.so.6"
        lib = ctypes.CDLL(name)
        lib.strdup.argtypes = [ctypes.c_char_p]
        lib.strdup.restype = ctypes.c_void_p
        lib.free.argtypes = [ctypes.c_void_p]
        lib.free.restype = None
        _libc = lib
    return _libc


class NrnRef(abc.ABC):
    """Abstract base class for NEURON references.

    Provides ``_push()`` and ``__getitem__``/``__setitem__`` interface.
    ``_push()`` is an internal protocol used by ``utils._push_args`` to place
    a typed pointer on NEURON's HOC stack before a C-API call.
    """

    __slots__ = ()

    @abc.abstractmethod
    def _push(self):
        raise NotImplementedError("_push() must be implemented by subclasses")

    def _pop(self):
        """Undo one successful ``_push`` during argument rollback."""
        from .api import _nrn_double_ptr_pop

        _nrn_double_ptr_pop()

    @abc.abstractmethod
    def __repr__(self):
        raise NotImplementedError("__repr__ must be implemented by subclasses")

    @abc.abstractmethod
    def __getitem__(self, idx):
        raise NotImplementedError("__getitem__ must be implemented by subclasses")

    @abc.abstractmethod
    def __setitem__(self, idx, value):
        raise NotImplementedError("__setitem__ must be implemented by subclasses")

    def __len__(self):
        # All current Ref types wrap a single scalar. Subclasses that wrap
        # arrays must override this.
        return 1

    def get(self):
        return self[0]

    def set(self, value):
        """Set ``self[0] = value``; returns value for chaining. Return type is float."""
        self[0] = value
        return value


class NrnRangeVarRef(NrnRef):
    """Pointer reference to a segment range variable, for HOC pointer args.

    Obtained via ``seg._ref_v`` / ``seg._ref_gnabar_hh``. Used as a HOC
    pointer (``netcon.record(seg._ref_v)``, ``vec.record(seg._ref_v)``).
    Retains non-owning HOC/template Section wrappers so temporary expressions
    such as ``cell.soma[0](0.5)._ref_v`` remain valid through argument push.
    Python-owned Sections stay weakly held so dropping their final owning
    wrapper preserves real NEURON's native-section lifetime.
    """

    __slots__ = ("_sec", "_sec_ref", "_x", "_name", "_sym")

    # _x is intentionally captured at construction. If nseg changes afterward,
    # x may land in a different or nonexistent segment; real NEURON has the same
    # reference-staleness behavior.
    def __init__(self, sec, x, name):
        from .api import _nrn_symbol

        self._sec = None
        self._sec_ref = None
        if sec.is_pysec():
            self._sec_ref = weakref.ref(sec)
        else:
            self._sec = sec
        self._x = x
        if not isinstance(name, bytes):
            name = name.encode("utf-8")
        self._name = name
        self._sym = _nrn_symbol(self._name)
        if self._sym is None:
            raise NameError(f"No such mechanism or variable: {self._name}")

    def __repr__(self):
        # Keep repr diagnostic even after a weakly-held Python-owned Section
        # has gone away; data access still raises through _section().
        sec = self._sec if self._sec is not None else self._sec_ref()
        return f"<NrnRef to {sec}({self._x}).{self._name.decode('utf-8')}>"

    def _section(self):
        if self._sec is not None:
            return self._sec
        sec = self._sec_ref()
        if sec is None:
            raise ReferenceError("Referenced section no longer exists")
        return sec

    def _push(self):
        from .api import _nrn_rangevar_push

        sec = self._section()
        # A section deleted through HOC keeps its wrapper alive but frees the C
        # section, so pushing its rangevar pointer would dereference freed state.
        # _check_alive raises RuntimeError instead.
        sec._check_alive()
        _nrn_rangevar_push(self._sym, sec._sec, self._x)

    def __getitem__(self, idx):
        from .api import _nrn_rangevar_get, _nrn_symbol

        if idx != 0:
            raise IndexError("Only index 0 is supported")
        sec = self._section()
        sec._check_alive()  # HOC-deleted section: RuntimeError, not SIGABRT.
        # TODO(gap-46): self._sym is already resolved in __init__; use it here
        # instead of calling _nrn_symbol again. The re-lookup is redundant and
        # slows hot reads. (_push already uses self._sym correctly.)
        sym = _nrn_symbol(self._name)
        if sym is None:
            raise NameError(f"No such mechanism or variable: {self._name}")
        return _nrn_rangevar_get(sym, sec._sec, self._x)

    def __setitem__(self, idx, value):
        from .api import _nrn_rangevar_set, _nrn_symbol

        if idx != 0:
            raise IndexError("Only index 0 is supported")
        sec = self._section()
        sec._check_alive()  # HOC-deleted section: RuntimeError, not SIGABRT.
        # TODO(gap-46): same redundant _nrn_symbol re-lookup as __getitem__;
        # use self._sym.
        sym = _nrn_symbol(self._name)
        if sym is None:
            raise NameError(f"No such mechanism or variable: {self._name}")
        _nrn_rangevar_set(sym, sec._sec, self._x, value)


class NrnVarRef(NrnRef):
    """Pointer reference to a global HOC double variable.

    Obtained via ``n._ref_t`` / ``n._ref_celsius`` etc. Used as a HOC
    pointer argument where a callable needs to read or write the global
    over time.
    """

    __slots__ = ("_name", "_sym")

    # Only doubles (subtype 0/2) are supported. Subtype 1 is the integer
    # global path (e.g. some HOC globals registered as ints) and would need
    # POINTER(c_int) handling everywhere _push and __getitem__ touch ctypes.
    def __init__(self, name):
        from .api import _nrn_symbol

        if not isinstance(name, bytes):
            name = name.encode("utf-8")
        self._name = name
        self._sym = _nrn_symbol(self._name)
        if self._sym is None:
            raise NameError(f"No such mechanism or variable: {self._name}")

    def __repr__(self):
        return f"<NrnRef to n.{self._name.decode()}>"

    def _push(self):
        # Push a raw double* onto NEURON's stack so the HOC function we're
        # about to call (typically Vector.record) can store the address for
        # later read. NrnRangeVarRef has nrn_rangevar_push for the equivalent
        # operation on segment-bound variables; there is no symbol_ptr_push
        # variant in the C API, so we fetch the address via _nrn_symbol_dataptr
        # and push it with the generic double_ptr_push.
        import ctypes
        from .api import (
            _nrn_symbol_dataptr,
            _nrn_double_ptr_push,
            _MIN_VALID_DATAPTR,
        )

        value_ptr = _nrn_symbol_dataptr(self._sym)
        addr = ctypes.cast(value_ptr, ctypes.c_void_p).value if value_ptr else None
        # A runtime (subtype-0) scalar on a libnrniv predating nrn#3815 returns a
        # bogus small-int address here; pushing/dereferencing it SIGSEGVs.
        # Detect it the same way top-level reads/writes do. Without
        # a real address there is nothing recordable to push, so fail cleanly.
        if not addr or addr < _MIN_VALID_DATAPTR:
            raise RuntimeError(
                f"cannot take a recordable pointer to '{self._name.decode()}' "
                "on this NEURON build (needs nrn#3815)"
            )
        _nrn_double_ptr_push(value_ptr)

    def __getitem__(self, idx):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        import ctypes
        from .api import _nrn_symbol_dataptr, _MIN_VALID_DATAPTR

        value_ptr = _nrn_symbol_dataptr(self._sym)
        addr = ctypes.cast(value_ptr, ctypes.c_void_p).value if value_ptr else None
        # For a bogus runtime-scalar address, route the read through the
        # guarded top-level path. n._ref_x[0] is by definition n.x.
        if not addr or addr < _MIN_VALID_DATAPTR:
            from . import NEURON

            return getattr(NEURON(), self._name.decode("utf-8"))
        return ctypes.cast(value_ptr, ctypes.POINTER(ctypes.c_double))[0]

    def __setitem__(self, idx, value):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        import ctypes
        from .api import _nrn_symbol_dataptr, _MIN_VALID_DATAPTR

        value_ptr = _nrn_symbol_dataptr(self._sym)
        addr = ctypes.cast(value_ptr, ctypes.c_void_p).value if value_ptr else None
        if not addr or addr < _MIN_VALID_DATAPTR:
            from . import NEURON

            setattr(NEURON(), self._name.decode("utf-8"), value)
            return
        ctypes.cast(value_ptr, ctypes.POINTER(ctypes.c_double))[0] = value


class NrnObjectPropertyRef(NrnRef):
    """Pointer reference to a HOC-object property (e.g. ``ic._ref_amp``).

    Obtained via ``obj._ref_<prop>``. Optional ``idx`` selects an entry
    of an array property. Holds the object as a weakref; raises
    ``ReferenceError`` if the host object is deleted.
    """

    __slots__ = ("_obj", "_name", "_idx")

    def __init__(self, obj, name, idx=None):
        self._obj = weakref.ref(obj)
        if not isinstance(name, bytes):
            name = name.encode("utf-8")
        self._name = name
        self._idx = idx

    def __repr__(self):
        obj = self._obj()
        clsname = getattr(obj, "_class_name", "?")
        name = self._name.decode("utf-8")
        if self._idx is None:
            return f"<NrnRef to {clsname}.{name}>"
        return f"<NrnRef to {clsname}.{name}[{self._idx}]>"

    def _push(self):
        # idx=None → scalar property; idx=int → array element at that index.
        # The outer __getitem__ index is always 0 — the outer interface is a
        # scalar ref to one element.
        obj = self._obj()
        if obj is None:
            raise ReferenceError("Referenced object no longer exists")
        # The weakref catches a GC'd wrapper, but a point process whose host
        # section was deleted through HOC keeps its wrapper alive while the C
        # node is freed; pushing/reading its property then SIGABRTs.
        # _check_host_alive is the same guard the direct ic.amp path uses.
        obj._check_host_alive()

        if self._idx is None:
            from .api import _property_push_checked

            _property_push_checked(obj._obj, self._name)
        else:
            from .api import _property_array_push_checked

            _property_array_push_checked(obj._obj, self._name, int(self._idx))

    def __getitem__(self, idx):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        obj = self._obj()
        if obj is None:
            raise ReferenceError("Referenced object no longer exists")
        obj._check_host_alive()  # HOC-deleted host: clean raise, not SIGABRT.

        if self._idx is None:
            from .api import _property_get_checked

            return _property_get_checked(obj._obj, self._name)
        else:
            from .api import _property_array_get_checked

            return _property_array_get_checked(obj._obj, self._name, int(self._idx))

    def __setitem__(self, idx, value):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        obj = self._obj()
        if obj is None:
            raise ReferenceError("Referenced object no longer exists")
        obj._check_host_alive()  # HOC-deleted host: clean raise, not SIGABRT.

        if self._idx is None:
            from .api import _ref_property_set_checked

            _ref_property_set_checked(obj._obj, self._name, value)
        else:
            from .api import _ref_property_array_set_checked

            _ref_property_array_set_checked(obj._obj, self._name, int(self._idx), value)


class NrnHocTemplateVarRef(NrnRef):
    """Safe scalar view of a user HOC template's numeric dataspace member.

    The public property-pointer API is only valid for C++ template properties;
    using it on a pure HOC template dereferences a null ``steer`` callback.
    Python-side reads and writes therefore use the same HOC trampolines as
    ``Object`` attribute access. A public dataspace-pointer API is still needed
    before this ref can be pushed to HOC calls such as ``Vector.record``.
    """

    __slots__ = ("_obj", "_name", "_idx")

    def __init__(self, obj, name, idx=None):
        self._obj = weakref.ref(obj)
        self._name = name
        self._idx = idx

    def _owner(self):
        obj = self._obj()
        if obj is None:
            raise ReferenceError("Referenced object no longer exists")
        return obj

    def _push(self):
        raise NotImplementedError(
            "pushing a reference to a user HOC-template variable requires "
            "a public object-dataspace pointer API (gap-34)"
        )

    def __getitem__(self, idx):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        from .object import _read_hoc_template_var

        return _read_hoc_template_var(self._owner(), self._name, self._idx)

    def __setitem__(self, idx, value):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        from .object import _write_hoc_template_var

        _write_hoc_template_var(self._owner(), self._name, value, self._idx)

    def __repr__(self):
        obj = self._obj()
        clsname = getattr(obj, "_class_name", "?")
        suffix = "" if self._idx is None else f"[{self._idx}]"
        return f"<NrnRef to HOC template {clsname}.{self._name}{suffix}>"


class NrnDoubleRef(NrnRef):
    """Fresh anonymous double cell — real NEURON's ``h.ref(3.0)``.

    Owns a Python ``c_double`` and pushes its address as a HOC ``$&1``
    pointer arg (``hoc_pushpx`` equivalent). HOC ``$&1 = v`` only writes the
    double in place — no allocation — so a Python-owned cell is safe here
    (unlike the string case). Settable from both sides via ``r[0]``.
    """

    __slots__ = ("_cell",)

    def __init__(self, value=0.0):
        self._cell = ctypes.c_double(float(value))

    def __repr__(self):
        return f"<NrnDoubleRef {self._cell.value!r}>"

    def _push(self):
        from .api import _nrn_double_ptr_push

        _nrn_double_ptr_push(
            ctypes.cast(ctypes.byref(self._cell), ctypes.POINTER(ctypes.c_double))
        )

    def __getitem__(self, idx):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        return self._cell.value

    def __setitem__(self, idx, value):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        self._cell.value = float(value)


class NrnStrRef(NrnRef):
    """Fresh anonymous string cell — real NEURON's ``h.ref('')``.

    Backs a libc-``strdup`` ``char*`` (a ``c_void_p`` cell) and pushes the
    cell's address as a HOC ``$s1`` string pointer arg (``hoc_pushstr``
    equivalent, ``nrn_str_push(char**)``). When a HOC function writes
    ``$s1 = "..."``, ``hoc_assign_str`` ``free()``s the current ``char*`` and
    replaces it with an ``emalloc``'d (``std::malloc``) copy; because the cell
    is libc-allocated the free is valid, and the new pointer lands back in the
    cell. Python-side ``r[0] = s`` mirrors that: ``free`` the old, ``strdup``
    the new. ``__del__`` frees the final buffer.
    """

    __slots__ = ("_cell",)

    def __init__(self, value=""):
        if isinstance(value, bytes):
            data = value
        else:
            data = str(value).encode("utf-8")
        libc = _get_libc()
        self._cell = ctypes.c_void_p(libc.strdup(data))

    def __repr__(self):
        return f"<NrnStrRef {self[0]!r}>"

    def _push(self):
        from .api import _nrn_str_push

        _nrn_str_push(
            ctypes.cast(ctypes.addressof(self._cell), ctypes.POINTER(ctypes.c_char_p))
        )

    def _pop(self):
        from .api import _nrn_str_pop

        _nrn_str_pop()

    def __getitem__(self, idx):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        if not self._cell.value:
            return ""
        return ctypes.string_at(self._cell.value).decode("utf-8")

    def __setitem__(self, idx, value):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        if isinstance(value, bytes):
            data = value
        else:
            data = str(value).encode("utf-8")
        libc = _get_libc()
        old = self._cell.value
        self._cell.value = libc.strdup(data)
        if old:
            libc.free(old)

    def __del__(self):
        # Free the final buffer. Guarded so a half-constructed instance
        # (strdup failed / _get_libc raised) doesn't raise in __del__.
        cell = getattr(self, "_cell", None)
        if cell is not None and cell.value:
            try:
                _get_libc().free(cell.value)
            except Exception:
                pass


class NrnObjectRef(NrnRef):
    """Fresh anonymous object cell — real NEURON's ``h.ref(obj)`` / ``h.ref(None)``.

    Holds an ``Object*`` cell (NULL for ``None``) and pushes the cell's address
    as a HOC objref pointer arg (``Object**``, via ``nrn_object_ptr_push``). When a HOC
    function assigns ``$o1 = x``, ``hoc_assign_obj`` unrefs the cell's old
    content and refs the new, so the invariant "the cell owns exactly one
    reference to its content (or holds NULL)" is preserved across the call. We
    claim that one reference at construction (``nrn_object_ref``) and release it
    in ``__del__`` (``nrn_object_unref``). Reading ``r[0]`` refs the content
    again before wrapping it, so the returned myneuron ``Object`` owns its own
    reference, independent of the cell's lifetime.

    Foreign Python objects use the active PythonObject provider and read back
    as the original Python value, not a second HOC wrapper.
    """

    __slots__ = ("_cell",)

    def __init__(self, value=None):
        self._cell = ctypes.c_void_p(0)
        self[0] = value

    def __repr__(self):
        return f"<NrnObjectRef {self[0]!r}>"

    def _push(self):
        from .api import _object_ptr_push

        _object_ptr_push(ctypes.byref(self._cell))

    def _pop(self):
        from .api import _discard_object_stack_value

        _discard_object_stack_value()

    def __getitem__(self, idx):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        ptr = self._cell.value
        if not ptr:
            return None
        # Conversion consumes a reference even for PythonObject payloads;
        # the cell must retain its own independently.
        from .api import _nrn_object_ref
        from .utils import _wrap_owned_object

        _nrn_object_ref(ptr)
        # Pass the raw int pointer, not c_void_p: Object._obj is an int
        # everywhere (nrn_object_pop's c_void_p restype yields an int), and
        # Object.__eq__ compares _obj, so wrapping in c_void_p would make a
        # round-tripped object compare unequal to its source.
        return _wrap_owned_object(ptr)

    def __setitem__(self, idx, value):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        from .api import _nrn_object_ref, _nrn_object_unref
        from .object import Object
        from .utils import _new_python_object

        old = self._cell.value
        if value is None:
            new_ptr = None
        elif isinstance(value, Object):
            new_ptr = value._obj
            _nrn_object_ref(new_ptr)  # cell claims a ref to the new content
        else:
            new_ptr = _new_python_object(value)
        # Acquire before replacing: failed conversion leaves the old cell intact.
        self._cell.value = new_ptr
        if old:
            _nrn_object_unref(old)  # release the old content

    def __del__(self):
        # Release the one reference the cell owns. Guarded against interpreter
        # teardown (libnrniv may be gone) and half-constructed instances, the
        # same way Object.__del__ is.
        if getattr(sys, "meta_path", None) is None or getattr(sys, "modules", None) is None:
            return
        cell = getattr(self, "_cell", None)
        if cell is None or not cell.value:
            return
        try:
            from .api import _nrn_object_unref

            _nrn_object_unref(cell.value)
        except Exception:
            pass
