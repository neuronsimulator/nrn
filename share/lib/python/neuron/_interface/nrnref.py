"""Reference types for NEURON's pointer-based variable tracking (_ref_ syntax).

Two families live here:

* **Binding refs** (`NrnRangeVarRef`, `NrnVarRef`, `NrnObjectPropertyRef`):
  a pointer to an existing symbol, property or range variable, obtained with
  ``_ref_`` (``seg._ref_v``, ``n._ref_t``, ``ic._ref_amp``).
* **Fresh-value refs** (`NrnDoubleRef`, `NrnStrRef`, `NrnObjectRef`): a new
  anonymous settable cell holding a value, like real NEURON's ``h.ref(x)``
  (double, string, or objref cell). Created by ``NEURON.ref`` (gap-37).
"""
import abc
import ctypes
import operator
import sys
import weakref

from .api import (
    _bump_model_epoch,
    _nrn_vector_data,
    _nrn_vector_capacity,
    _discard_object_stack_value,
    _nrn_double_ptr_pop,
    _nrn_double_ptr_push,
    _nrn_object_ref,
    _nrn_rangevar_get,
    _nrn_rangevar_push,
    _nrn_rangevar_push_nothrow,
    _nrn_rangevar_set,
    _nrn_section_is_active,
    _nrn_str_pop,
    _nrn_str_push,
    _nrn_symbol,
    _nrn_symbol_dataptr,
    _object_ptr_push,
    _property_array_get_checked,
    _property_array_push_checked,
    _property_get_checked,
    _property_push_checked,
    _ref_property_array_set_checked,
    _ref_property_set_checked,
    _top_level_symbol,
    _ERR_BUF,
    _ERR_BUF_SIZE,
    HOC_LOCK,
    _as_double,
    _nrn_property_element_ptr,
    _nrn_rangevar_handle_str,
    _nrn_symbol_is_array,
    symbol_array_dims,
    _PACKAGE,
    _str_assign,
)
from . import api as _api

# The count a _push raises when its caller keeps none.
_SCRATCH = [0]


def _symbol_info(name):
    return _PACKAGE.sections._get_symbol_info(name.decode("utf-8"))


def refuse_pickle(self, protocol):
    """``__reduce_ex__`` of a wrapper that is a HocObject in NEURON.

    NEURON pickles only Vectors. Python's default would pickle the slots,
    pointers and owned cells among them, and a copy would free or unref what
    only the original owns (copying ``n.ref("abc")`` freed its string twice).
    copy and deepcopy fall back to this too, so they raise the same error.
    """
    raise TypeError("HocObject: Only Vector instance can be pickled")


def _as_index(idx, type_message):
    """Return *idx* as an int, as NEURON reads a subscript (PyLong_AsLong).

    That is the index protocol: a numpy integer or a bool is an index, a float
    is not and raises TypeError(*type_message*), with ``{}`` standing for its
    type's name.
    """
    try:
        return operator.index(idx)
    except TypeError:
        raise TypeError(type_message.format(type(idx).__name__)) from None


def _array_index(idx, size, type_message):
    """Return *idx* as an index into a HOC array of *size*, or None if outside.

    As NEURON's araychk, a negative index is outside the array.
    """
    i = _as_index(idx, type_message)
    return i if 0 <= i < size else None


def _subscripts(idx):
    """The subscripts of a member element, as a tuple: *idx* is None (a
    scalar), one index or a tuple of them."""
    if idx is None:
        return ()
    return (idx,) if isinstance(idx, int) else tuple(idx)


def _flat_index(indices, dims):
    """Position of the element at *indices* in HOC's row-major storage of an
    array of shape *dims*."""
    flat = 0
    for i, d in zip(indices, dims):
        flat = flat * d + i
    return flat


def _sequence_index(idx, size, type_message):
    """As _array_index, but a negative index counts from the end.

    For a range variable array (``seg.xg[-1]``), which NEURON indexes as a
    Python sequence.
    """
    i = _as_index(idx, type_message)
    if i < 0:
        i += size
    return i if 0 <= i < size else None


class NrnRef(abc.ABC):
    """Abstract base class for NEURON references.

    Provides ``_push()`` and ``__getitem__``/``__setitem__`` interface.
    ``_push()`` is an internal protocol used by ``utils._push_args`` to place
    a typed pointer on NEURON's HOC stack before a C-API call.
    """

    __slots__ = ()

    __reduce_ex__ = refuse_pickle

    # What _push leaves on the stack, for utils._pop_args: "double_ptr",
    # "str" or "object".
    _slot = "double_ptr"

    @abc.abstractmethod
    def _push(self, count=_SCRATCH):
        """Push the pointer. ``count[0]`` is raised just before the final
        ctypes push, with nothing in between: an interrupt is raised as a C
        call returns, so counting after the push could miss it (gap-84)."""
        raise NotImplementedError("_push() must be implemented by subclasses")

    def _pop(self):
        """Undo one successful ``_push`` during argument rollback."""
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


def _store_through_pointer(sym, sec_ptr, x, value):
    """Write a range variable's own storage, as NEURON's reference does.

    ``nrn_rangevar_set`` also rewrites what a ``diam`` write implies (3-D
    points, areas) on cores that do so; a reference store only changes the
    node's value.
    """
    value = _as_double(value)
    with HOC_LOCK:  # the pop must take this push (gap-72)
        _nrn_rangevar_push(sym, sec_ptr, x)
        raw = _nrn_double_ptr_pop()
    if not raw:
        raise RuntimeError("could not acquire a pointer to the range variable")
    raw[0] = value


class NrnRangeVarRef(NrnRef):
    """Pointer reference to a segment range variable, for HOC pointer args.

    Obtained via ``seg._ref_v`` / ``seg._ref_gnabar_hh``. Used as a HOC
    pointer (``netcon.record(seg._ref_v)``, ``vec.record(seg._ref_v)``).
    HOC/template-owned Section wrappers are held strongly, so temporaries such
    as ``cell.soma[0](0.5)._ref_v`` stay valid through argument push (gap-50).
    Python-owned Sections are held weakly, so dropping the last owning wrapper
    still frees the section as in real NEURON.
    """

    __slots__ = ("_sec", "_sec_ref", "_x", "_name", "_sym", "_nseg", "_index")

    # _x is captured at construction. If nseg changes afterward, x may land in
    # a different or nonexistent segment; real NEURON behaves the same way.
    # _index is the element of a range variable array (None for a scalar);
    # NrnRangeArrayRef makes these for seg._ref_<array>[i].
    def __init__(self, sec, x, name, index=None):
        self._index = index
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
        # NEURON's handle follows its node through nseg changes and dies with
        # it (sections.follow_node); _check_nseg moves _x along.
        self._nseg = _PACKAGE.sections.current_nseg_generation(sec._sec)

    def _check_nseg(self, sec):
        # Also catches an nseg change made from HOC code (sync_nseg), which
        # NEURON's handle sees: its node may be gone (datahandle test_1).
        sections = _PACKAGE.sections
        sec_ptr = sec._sec
        sections.sync_nseg(sec_ptr)
        generation = sections.NSEG_GENERATION.get(sec_ptr, 0)
        if generation == self._nseg:
            return
        x = sections.follow_node(sec_ptr, self._nseg, self._x, bool(sec.orientation()))
        if x is None:
            raise ValueError("Invalid data_handle")
        self._x = x
        self._nseg = generation

    def __repr__(self):
        # repr stays usable after a weakly held Section is gone; data access
        # still raises through _section(). A reference that can no longer
        # be read (section deleted, mechanism uninserted) says "died", as
        # NEURON's data_handle repr does.
        sec = self._sec if self._sec is not None else self._sec_ref()
        target = f"{sec}({self._x}).{self._name.decode('utf-8')}"
        if self._index is not None:
            target += f"[{self._index}]"
        try:
            self._checked_target()
        except (ReferenceError, RuntimeError, ValueError):
            return f"<NrnRef to {target}: died>"
        described = self._handle_text(sec)
        return described if described is not None else f"<NrnRef to {target}>"

    def _handle_text(self, sec):
        """NEURON's text for the reference, from the core (gap-65), or None."""

        if _nrn_rangevar_handle_str is None or sec is None:
            return None
        buf = ctypes.create_string_buffer(256)
        length = _nrn_rangevar_handle_str(
            self._sym, sec._sec, self._x, self._index or 0, buf, len(buf)
        )
        if length < 0:
            return None
        if length >= len(buf):
            buf = ctypes.create_string_buffer(length + 1)
            _nrn_rangevar_handle_str(
                self._sym, sec._sec, self._x, self._index or 0, buf, len(buf)
            )
        return buf.value.decode("utf-8", errors="replace")

    def _section(self, error=ValueError, message="Invalid data_handle"):
        """The section, or NEURON's error for a reference whose section Python
        deleted: the same as one HOC deleted."""
        if self._sec is not None:
            return self._sec
        sec = self._sec_ref()
        if sec is None:
            raise error(message)
        return sec

    def _push(self, count=_SCRATCH):
        sec = self._section(RuntimeError, "Invalid pointer")
        # A section deleted through HOC keeps its wrapper but frees the C
        # section. Raise NEURON's HOC error for a dead pointer argument
        # instead of pushing freed state.
        if not _nrn_section_is_active(sec._sec):
            raise RuntimeError("Invalid pointer")
        self._check_nseg(sec)
        name = self._name.decode("utf-8")
        if not 0.0 < self._x < 1.0:  # a section loop throws in the push
            _PACKAGE.sections._ensure_rangevar_topology(sec._sec, self._x, name)
        if _nrn_rangevar_push_nothrow is not None:
            # Takes the array index directly, and reports a failure (a
            # mechanism that is no longer inserted, i_membrane_ without
            # use_fast_imem) as an error instead of a HOC error, which would
            # cross ctypes as a C++ exception and abort the process.
            err = _ERR_BUF()
            sym, sec_ptr, x, index = self._sym, sec._sec, self._x, self._index or 0
            count[0] += 1
            if _nrn_rangevar_push_nothrow(sym, sec_ptr, x, index, err, _ERR_BUF_SIZE):
                count[0] -= 1  # nothing was pushed
                raise RuntimeError(err.value.decode("utf-8", "replace"))
            return
        # Without the nothrow push, nrn_rangevar_push is the only one, and a HOC
        # error in it aborts the process. On an array symbol it reads the index
        # from the HOC stack, which cannot be pushed here.
        if _symbol_info(self._name)[2]:
            raise RuntimeError(
                f"a pointer to {name}[{self._index or 0}] needs "
                "nrn_rangevar_push_nothrow, which this NEURON lacks"
            )
        # Its other error is a mechanism that is not inserted here (uninserted
        # after the reference was taken); check that first.

        if _PACKAGE.sections._missing_owner(sec, name) is not None:
            raise RuntimeError(f"{name}: mechanism is not inserted at this location")
        sym, sec_ptr, x = self._sym, sec._sec, self._x
        count[0] += 1
        _nrn_rangevar_push(sym, sec_ptr, x)

    def _checked_target(self):
        """Return (section, name, kind) or raise before the core is touched.

        nrn_rangevar_get/set dereference unchecked. The core throws through
        ctypes and aborts the process after the mechanism is uninserted, on an
        array symbol, or through an unset POINTER.
        """

        sec = self._section()
        if not _nrn_section_is_active(sec._sec):
            # HOC deleted the section: NEURON's reference is a dead
            # data_handle. Reading it here would SIGABRT.
            raise ValueError("Invalid data_handle")
        self._check_nseg(sec)
        name = (
            self._name.decode("utf-8") if isinstance(self._name, bytes) else self._name
        )
        if _PACKAGE.sections._missing_owner(sec, name) is not None:
            raise ValueError("Invalid data_handle")
        info = _PACKAGE.sections._get_symbol_info(name)
        if info is None:
            raise NameError(f"No such mechanism or variable: {name}")
        if info[2]:
            return sec, name, ("array", info[3])
        if _PACKAGE.sections._is_pointer_rangevar(name, info[0]):
            return sec, name, "pointer"
        return sec, name, info[0]

    def _element(self, idx, name, length):
        # NEURON's element pointer indexes relative to itself:
        # seg._ref_xg[0][1] is xg[1], and seg._ref_xg[1][-1] is xg[0].
        offset = _as_index(idx, "indices must be integers, not {}")
        base = self._index or 0
        element = base + offset
        if not 0 <= element < length:
            raise IndexError(f"{name}[{base}] next_array_element({idx}): out of range")
        return element

    def __getitem__(self, idx):
        sec, name, kind = self._checked_target()
        if isinstance(kind, tuple):  # ("array", length)
            length = kind[1]
            element = self._element(idx, name, length)
            return _PACKAGE.sections._ArrayRangeVar(sec, self._x, name, length)[element]
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        if kind == "pointer":
            return _PACKAGE.sections._pointer_rangevar_get(sec, self._x, name)
        if not 0.0 < self._x < 1.0:  # a section loop throws in the read
            _PACKAGE.sections._ensure_rangevar_topology(sec._sec, self._x, name)
        return _nrn_rangevar_get(kind, sec._sec, self._x)

    def __setitem__(self, idx, value):
        if value.__class__ is not float and value.__class__ is not int:
            value = _as_double(value)  # user code may run: before the checks
        sec, name, kind = self._checked_target()
        if isinstance(kind, tuple):  # ("array", length)
            length = kind[1]
            element = self._element(idx, name, length)
            _PACKAGE.sections._ArrayRangeVar(sec, self._x, name, length)[
                element
            ] = value
            return
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        if kind == "pointer":
            _PACKAGE.sections._pointer_rangevar_set(sec, self._x, name, value)
            return
        if not 0.0 < self._x < 1.0:  # a section loop throws in the write
            _PACKAGE.sections._ensure_rangevar_topology(sec._sec, self._x, name)
        if name == "diam":
            _store_through_pointer(kind, sec._sec, self._x, value)
            return
        _nrn_rangevar_set(kind, sec._sec, self._x, value)


class NrnRangeArrayRef(NrnRef):
    """``seg._ref_<array>`` / ``seg.mech._ref_<array>``: ``[i]`` points at element ``i``.

    Like NEURON's ``nrn.RangeVar``, it has a len() and iterates over the
    element pointers, but is not itself a pointer: ``vec.record(seg._ref_xg)``
    fails where ``vec.record(seg._ref_xg[1])`` records ``xg[1]``.
    """

    __slots__ = ("_sec", "_x", "_name")

    def __reduce_ex__(self, protocol):
        raise TypeError("cannot pickle 'nrn.RangeVar' object")

    def __init__(self, sec, x, name):
        self._sec = sec
        self._x = x
        self._name = name

    def __len__(self):
        return _symbol_info(self._name.encode("utf-8"))[3]

    def _resolve(self, idx):
        i = _sequence_index(idx, len(self), "indices must be integers, not {}")
        if i is None:
            raise IndexError(self._name)
        return i

    def __getitem__(self, idx):
        return NrnRangeVarRef(self._sec, self._x, self._name, self._resolve(idx))

    def __setitem__(self, idx, value):
        self[idx][0] = value

    def __iter__(self):
        for i in range(len(self)):
            yield self[i]

    def _push(self, count=_SCRATCH):
        raise RuntimeError(
            f"{self._name} is a range variable array; pass a pointer to one "
            f"element, e.g. _ref_{self._name}[0]"
        )

    def __repr__(self):
        return f"<NrnRef to {self._sec}({self._x}).{self._name}[]>"


class NrnVarRef(NrnRef):
    """Pointer reference to a global HOC double variable.

    Obtained via ``n._ref_t`` / ``n._ref_celsius`` etc. Used as a HOC
    pointer argument where a callable needs to read or write the global
    over time.
    """

    __slots__ = ("_name", "_sym")

    # Only doubles (subtype 0/2) are supported. Subtype 1 (integer globals)
    # would need POINTER(c_int) handling in _push and __getitem__.
    def __init__(self, name):
        if not isinstance(name, bytes):
            name = name.encode("utf-8")
        self._name = name
        self._sym = _top_level_symbol(self._name)
        if self._sym is None:
            raise NameError(f"No such mechanism or variable: {self._name}")

    def __repr__(self):
        return f"<NrnRef to n.{self._name.decode()}>"

    def _push(self, count=_SCRATCH):
        # Push a raw double* so the HOC function being called (typically
        # Vector.record) can store the address and read it later. The C API
        # has no symbol-pointer push: fetch the address with
        # nrn_symbol_dataptr and push it with double_ptr_push.

        value_ptr = _nrn_symbol_dataptr(self._sym)
        if not value_ptr:
            raise RuntimeError(
                f"cannot take a recordable pointer to '{self._name.decode()}' "
                "on this NEURON build (needs nrn#3815)"
            )
        count[0] += 1
        _nrn_double_ptr_push(value_ptr)

    def __getitem__(self, idx):
        if idx != 0:
            raise IndexError("Only index 0 is supported")

        value_ptr = _nrn_symbol_dataptr(self._sym)
        if not value_ptr:
            return getattr(_PACKAGE._n, self._name.decode("utf-8"))
        return value_ptr[0]

    def __setitem__(self, idx, value):
        if idx != 0:
            raise IndexError("Only index 0 is supported")

        value_ptr = _nrn_symbol_dataptr(self._sym)
        if not value_ptr:
            setattr(_PACKAGE._n, self._name.decode("utf-8"), value)
            return
        value_ptr[0] = value


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

    def _owner(self):
        """The object, checked before its property data is touched.

        The weakref catches a collected wrapper. A point process that is in no
        section (never placed, or its section is deleted) has no data, and
        reading, writing or pushing its property would abort. Same guard as the
        direct ic.amp path, where NEURON says "No value"; its reference is a
        dead data_handle.
        """
        obj = self._obj()
        if obj is None:
            raise ReferenceError("Referenced object no longer exists")
        try:
            obj._check_located()
        except TypeError:
            raise ValueError("Invalid data_handle") from None
        return obj

    def _push(self, count=_SCRATCH):
        # idx=None: scalar property; idx=int: that array element.
        try:
            obj = self._owner()
        except ValueError:
            raise RuntimeError("Invalid pointer") from None  # NEURON's HOC error
        if self._idx is None:
            _property_push_checked(obj._obj, self._name, count)
        else:
            _property_array_push_checked(obj._obj, self._name, int(self._idx), count)

    def __getitem__(self, idx):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        obj = self._owner()
        if self._idx is None:
            return _property_get_checked(obj._obj, self._name)
        else:
            return _property_array_get_checked(obj._obj, self._name, int(self._idx))

    def __setitem__(self, idx, value):
        if value.__class__ is not float and value.__class__ is not int:
            value = _as_double(value)  # user code may run: before the checks
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        obj = self._owner()
        if self._idx is None:
            _ref_property_set_checked(obj._obj, self._name, value)
        else:
            _ref_property_array_set_checked(obj._obj, self._name, int(self._idx), value)


def reference(target):
    """What NEURON shows a Python GUI for a pointer argument (a HOC Pointer), or
    for the string HOC holds at an address."""
    if isinstance(target, int):
        ref = NrnStrRef.__new__(_HocStrRef)
        ref._cell = ctypes.cast(target, ctypes.POINTER(ctypes.c_void_p)).contents
        return ref
    ref = NrnObjectPropertyRef(target, b"val")
    ref._obj = lambda: target  # keeps it
    return ref


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

    def _push(self, count=_SCRATCH):
        raise NotImplementedError(
            "pushing a reference to a user HOC-template variable requires "
            "a public object-dataspace pointer API (gap-34)"
        )

    def __getitem__(self, idx):
        if idx != 0:
            raise IndexError("Only index 0 is supported")

        return _PACKAGE.object._read_hoc_template_var(
            self._owner(), self._name, self._idx
        )

    def __setitem__(self, idx, value):
        if idx != 0:
            raise IndexError("Only index 0 is supported")

        _PACKAGE.object._write_hoc_template_var(
            self._owner(), self._name, value, self._idx
        )

    def __repr__(self):
        obj = self._obj()
        clsname = getattr(obj, "_class_name", "?")
        suffix = "".join(f"[{i}]" for i in _subscripts(self._idx))
        return f"<NrnRef to HOC template {clsname}.{self._name}{suffix}>"


class NrnHocTemplateStrRef(NrnRef):
    """``obj._ref_<strdef>`` of a HOC template object: ``[0]`` reads and
    writes the strdef, as NEURON's string reference does.

    It cannot be passed to HOC: that needs the strdef's ``char**``, which no
    C API gives. Holds the object as a weakref.
    """

    __slots__ = ("_obj", "_name")

    def __init__(self, obj, name):
        self._obj = weakref.ref(obj)
        self._name = name

    def _owner(self):
        obj = self._obj()
        if obj is None:
            raise ReferenceError("Referenced object no longer exists")
        return obj

    def _push(self, count=_SCRATCH):
        raise NotImplementedError(
            f"a reference to strdef '{self._name}' of a HOC template object "
            "cannot be passed to HOC"
        )

    def __getitem__(self, idx):
        if idx != 0:
            raise IndexError("Only index 0 is supported")

        return _PACKAGE.object._read_hoc_template_string(self._owner(), self._name)

    def __setitem__(self, idx, value):
        if idx != 0:
            raise IndexError("Only index 0 is supported")

        _PACKAGE.object._write_hoc_template_string(self._owner(), self._name, value)

    def __repr__(self):
        obj = self._obj()
        clsname = getattr(obj, "_class_name", "?")
        return f"<NrnRef to HOC template {clsname}.{self._name}>"


class NrnTopStrRef(NrnRef):
    """``h._ref_<strdef>`` of a top-level strdef: ``[0]`` reads and writes
    the string, as NEURON's string reference does.

    It cannot be passed to HOC: that needs the strdef's ``char**``, which no
    C API gives.
    """

    __slots__ = ("_name",)

    def __init__(self, name):
        self._name = name

    def _push(self):
        raise NotImplementedError(
            f"a reference to strdef '{self._name}' cannot be passed to HOC"
        )

    def __getitem__(self, idx):
        if idx != 0:
            raise IndexError("index for hoc ref must be 0")

        return getattr(_PACKAGE._n, self._name)

    def __setitem__(self, idx, value):
        if idx != 0:
            raise IndexError("index for hoc ref must be 0")

        setattr(_PACKAGE._n, self._name, value)

    def __repr__(self):
        return f'<hoc ref pstr "{self[0]}">'  # NEURON's text


class NrnVectorElementRef(NrnRef):
    """Pointer to element ``i`` of a Vector, ``vec._ref_x[i]``.

    Pushed as the address in the Vector's data, as NEURON's pointer is; it
    follows the Vector only until the Vector is resized. Holds the Vector.
    """

    __slots__ = ("_vec", "_index")

    def __init__(self, vec, index):
        self._vec = vec
        self._index = index

    def _address(self):
        size = _nrn_vector_capacity(self._vec._obj)
        if not 0 <= self._index < size:
            raise IndexError(f"Vector index {self._index} out of range for size {size}")
        return ctypes.cast(
            ctypes.addressof(_nrn_vector_data(self._vec._obj).contents)
            + 8 * self._index,
            ctypes.POINTER(ctypes.c_double),
        )

    def _push(self, count=_SCRATCH):
        address = self._address()
        count[0] += 1
        _nrn_double_ptr_push(address)

    def __getitem__(self, idx):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        return self._address()[0]

    def __setitem__(self, idx, value):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        self._address()[0] = float(value)

    def __repr__(self):
        return f"<NrnRef to {self._vec.hname()}.x[{self._index}]>"


def _double_array_data(name, dims):
    """Base ``double*`` of top-level ``double name[...]`` with shape ``dims``.

    Looked up on every access: redeclaring the array reallocates it.
    """
    sym = _top_level_symbol(name.encode("utf-8"))
    if not sym or not _nrn_symbol_is_array(sym):
        raise TypeError(f"'{name}' is no longer an array")
    current = symbol_array_dims(sym)
    if current != dims:
        raise TypeError(f"'{name}' was redeclared with dimensions {current}")
    data = _nrn_symbol_dataptr(sym)
    if not data:
        raise RuntimeError(f"no data pointer for HOC array '{name}'")
    return data


class NrnDoubleArrayElementRef(NrnRef):
    """Pointer to an element of a top-level double array, ``n._ref_a[i][j]``.

    As NEURON's pointer, ``[k]`` reads the element ``k`` places further on in
    the array's row-major storage (``_ref_a[0][0][6]`` is ``a[1][2]`` for
    ``double a[3][4]``); unlike NEURON's raw pointer, reading past the end of
    the array raises.
    """

    __slots__ = ("_name", "_dims", "_flat")

    def __init__(self, name, dims, flat):
        self._name = name
        self._dims = dims
        self._flat = flat

    def _address(self, offset=0):
        offset = _as_index(offset, "indices must be integers, not {}")
        data = _double_array_data(self._name, self._dims)
        total = 1
        for d in self._dims:
            total *= d
        element = self._flat + offset
        if not 0 <= element < total:
            raise IndexError(self._name)
        return ctypes.cast(
            ctypes.addressof(data.contents) + 8 * element,
            ctypes.POINTER(ctypes.c_double),
        )

    def _push(self, count=_SCRATCH):
        address = self._address()
        count[0] += 1
        _nrn_double_ptr_push(address)

    def __getitem__(self, idx):
        return self._address(idx)[0]

    def __setitem__(self, idx, value):
        self._address(idx)[0] = float(value)

    def __repr__(self):
        return f"<NrnRef to {self._name}[{self._flat}]>"


class NrnMatrixElementRef(NrnRef):
    """Pointer to element ``(i, j)`` of a Matrix, ``m._ref_x[i][j]``.

    As NEURON's pointer, ``[k]`` reads the element ``k`` places further on in
    row-major order (``_ref_x[0][0][ncol]`` is ``x[1][0]``), within the
    matrix. Pushing it needs nrn_property_element_ptr (nrn#3900). Holds the
    Matrix.
    """

    __slots__ = ("_matrix", "_flat")

    def __init__(self, matrix, row, col):
        self._matrix = matrix
        self._flat = row * int(matrix.ncol()) + col

    def _element(self, offset=0):
        offset = _as_index(offset, "indices must be integers, not {}")
        nrow, ncol = int(self._matrix.nrow()), int(self._matrix.ncol())
        flat = self._flat + offset
        if not 0 <= flat < nrow * ncol:
            raise IndexError(f"{self._matrix.hname()}.x")
        return divmod(flat, ncol)

    def _push(self, count=_SCRATCH):
        row, col = self._element()
        if _nrn_property_element_ptr is None:
            raise RuntimeError(
                "a pointer to a Matrix element needs nrn_property_element_ptr, "
                "which this NEURON lacks (nrn#3900)"
            )
        indices = (ctypes.c_int * 2)(row, col)
        ptr = ctypes.POINTER(ctypes.c_double)()
        err = _ERR_BUF()
        if _nrn_property_element_ptr(
            self._matrix._obj,
            b"x",
            indices,
            2,
            ctypes.byref(ptr),
            err,
            _ERR_BUF_SIZE,
        ):
            raise RuntimeError(err.value.decode("utf-8", "replace"))
        count[0] += 1
        _nrn_double_ptr_push(ptr)

    def __getitem__(self, idx):
        return float(self._matrix.getval(*self._element(idx)))

    def __setitem__(self, idx, value):
        self._matrix.setval(*self._element(idx), float(value))

    def __repr__(self):
        row, col = divmod(self._flat, max(int(self._matrix.ncol()), 1))
        return f"<NrnRef to {self._matrix.hname()}.x[{row}][{col}]>"


class NrnArrayPropertyRef(NrnRef):
    """``obj._ref_<array>``: ``[i]`` is a reference to element ``i``.

    As in NEURON, pushed bare it points at element 0, and it has no len().
    """

    __slots__ = ("_owner", "_name", "_size")

    def __reduce_ex__(self, protocol):
        # Pickles as its owner allows: NEURON pickles vec._ref_x (loading it
        # back as the Vector) and refuses any other owner.
        return object.__reduce_ex__(self, protocol)

    def __init__(self, owner, name, size):
        self._owner = owner
        self._name = name
        self._size = size

    def __len__(self):
        raise TypeError("Most HocObject have no len()")

    def __getitem__(self, idx):
        i = _array_index(idx, self._size, "array reference index must be an integer")
        if i is None:
            raise IndexError(
                f"index {idx} out of range [0, {self._size}) for {self._name}"
            )
        return self._owner._property_ref(self._name, idx=i)

    def __setitem__(self, idx, value):
        self[idx][0] = value

    def _push(self, count=_SCRATCH):
        self[0]._push(count)

    def _pop(self):
        _nrn_double_ptr_pop()

    def __repr__(self):
        # Not owner.hname(): a template member named hname would be that.

        return (
            f"<NrnRef to {_PACKAGE.object._instance_hname(self._owner)}.{self._name}>"
        )


class NrnDoubleRef(NrnRef):
    """Fresh anonymous double cell, like real NEURON's ``h.ref(3.0)``.

    Owns a Python ``c_double`` and pushes its address as a HOC ``$&1``
    pointer arg (``hoc_pushpx`` equivalent). HOC ``$&1 = v`` writes in place,
    so a Python-owned cell is safe (unlike the string case). Settable from
    both sides via ``r[0]``.
    """

    __slots__ = ("_cell",)

    def __init__(self, value=0.0):
        self._cell = ctypes.c_double(float(value))

    def __repr__(self):
        return f"<NrnDoubleRef {self._cell.value!r}>"

    def _push(self, count=_SCRATCH):
        ptr = ctypes.cast(ctypes.byref(self._cell), ctypes.POINTER(ctypes.c_double))
        count[0] += 1
        _nrn_double_ptr_push(ptr)

    def __getitem__(self, idx):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        return self._cell.value

    def __setitem__(self, idx, value):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        self._cell.value = float(value)


class NrnRawDoubleRef(NrnRef):
    """A reference to a double at a raw address, e.g. a numpy array element.

    NEURON's ``numpy_element_ref(array, i)`` (no bounds checking). Holds the
    owner (the array) so the memory outlives the reference.
    """

    __slots__ = ("_address", "_owner")

    def __init__(self, address, owner=None):
        self._address = int(address)
        self._owner = owner

    def __repr__(self):
        return f"<NrnRawDoubleRef 0x{self._address:x}>"

    def _push(self, count=_SCRATCH):
        ptr = ctypes.cast(self._address, ctypes.POINTER(ctypes.c_double))
        count[0] += 1
        _nrn_double_ptr_push(ptr)

    def __getitem__(self, idx):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        return ctypes.cast(self._address, ctypes.POINTER(ctypes.c_double))[0]

    def __setitem__(self, idx, value):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        ctypes.cast(self._address, ctypes.POINTER(ctypes.c_double))[0] = float(value)


def numpy_element_ref(numpy_array, index):
    """A HOC reference to element `index` of a contiguous float64 array, as
    neuron.numpy_element_ref. No bounds checking, as in NEURON."""
    address = numpy_array.ctypes.data + int(index) * ctypes.sizeof(ctypes.c_double)
    return NrnRawDoubleRef(address, numpy_array)


class NrnStrRef(NrnRef):
    """Fresh anonymous string cell, like real NEURON's ``h.ref('')``.

    Holds a ``char*`` (a ``c_void_p`` cell) and pushes the cell's address as a
    HOC ``$s1`` string pointer arg (``nrn_str_push(char**)``). HOC frees and
    replaces the string on ``$s1 = "..."``, so it is NEURON's to allocate and
    free, here too (``nrn_str_assign``). See ``tests/regression/test_hoc_ref.py``
    ``test_str_ref_repeated_hoc_writeback_no_corruption``.
    """

    __slots__ = ("_cell",)

    def __init__(self, value=""):
        if isinstance(value, bytes):
            data = value
        else:
            data = str(value).encode("utf-8")
        self._cell = ctypes.c_void_p()
        _str_assign(self._cell, data)

    def __repr__(self):
        return f"<NrnStrRef {self[0]!r}>"

    _slot = "str"

    def _push(self, count=_SCRATCH):
        ptr = ctypes.cast(ctypes.addressof(self._cell), ctypes.POINTER(ctypes.c_char_p))
        count[0] += 1
        _nrn_str_push(ptr)

    def _pop(self):
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
        _str_assign(self._cell, data)

    def __del__(self):
        cell = getattr(self, "_cell", None)
        if cell is not None and cell.value:
            _str_assign(cell, None)


class _HocStrRef(NrnStrRef):
    """A string HOC holds."""

    __slots__ = ()

    def __del__(self):
        pass


def _release_object(obj):
    """Drop a reference cell's reference to HOC object *obj*.

    The last reference to a template cell (or a List of them) deletes its
    sections, so that moves the model epoch, as releasing a wrapper does (gap-68).
    """

    # Read the class before the object can go.
    class_name = (_api._nrn_class_name(obj) or b"").decode("utf-8", "replace")
    _api._nrn_object_unref(obj)
    if _PACKAGE.object._may_own_sections(class_name):
        _bump_model_epoch()


class NrnObjectRef(NrnRef):
    """Fresh anonymous object cell, like real NEURON's ``h.ref(obj)`` / ``h.ref(None)``.

    Holds an ``Object*`` cell (NULL for ``None``) and pushes its address as a
    HOC objref pointer arg (``Object**``, via ``nrn_object_ptr_push``).
    Invariant: the cell owns exactly one reference to its content, or holds
    NULL. ``hoc_assign_obj`` (``$o1 = x``) unrefs the old content and refs the
    new, preserving it. The cell takes its reference at construction
    (``nrn_object_ref``) and releases it in ``__del__`` (``nrn_object_unref``).
    ``r[0]`` refs the content again before wrapping, so the returned ``Object``
    owns its own reference, independent of the cell.

    Foreign Python objects use the active PythonObject provider and read back
    as the original Python value, not a second HOC wrapper.
    """

    __slots__ = ("_cell",)

    def __init__(self, value=None):
        self._cell = ctypes.c_void_p(0)
        self[0] = value

    def __repr__(self):
        return f"<NrnObjectRef {self[0]!r}>"

    _slot = "object"

    def _push(self, count=_SCRATCH):
        _object_ptr_push(ctypes.byref(self._cell), count)

    def _pop(self):
        _discard_object_stack_value()

    def __getitem__(self, idx):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        ptr = self._cell.value
        if not ptr:
            return None
        # Conversion consumes a reference even for PythonObject payloads; the
        # cell keeps its own.

        _nrn_object_ref(ptr)
        # Pass the raw int, not c_void_p: Object._obj is an int everywhere
        # (nrn_object_pop's c_void_p restype yields one) and Object.__eq__
        # compares it, so a c_void_p would make a round-tripped object
        # unequal to its source.
        return _PACKAGE.utils._wrap_owned_object(ptr)

    def __setitem__(self, idx, value):
        if idx != 0:
            raise IndexError("Only index 0 is supported")

        old = self._cell.value
        if value is None:
            new_ptr = None
        elif isinstance(value, _PACKAGE.object.Object):
            new_ptr = value._obj
            _nrn_object_ref(new_ptr)  # cell claims a ref to the new content
        else:
            new_ptr = _PACKAGE.utils._new_python_object(value)
        # Acquire before replacing: failed conversion leaves the old cell intact.
        self._cell.value = new_ptr
        if old:
            _release_object(old)  # release the old content

    def __del__(self):
        # Release the cell's one reference. Guarded like _released_base's
        # __del__ against interpreter teardown and half-constructed instances.
        if (
            getattr(sys, "meta_path", None) is None
            or getattr(sys, "modules", None) is None
        ):
            return
        cell = getattr(self, "_cell", None)
        if cell is None or not cell.value:
            return
        try:
            _release_object(cell.value)
        except Exception:
            pass
