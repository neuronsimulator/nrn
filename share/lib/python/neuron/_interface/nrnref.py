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
import numbers
import sys
import weakref

from .api import (
    _nrn_vector_data,
    _nrn_vector_capacity,
    _MIN_VALID_DATAPTR,
    _discard_object_stack_value,
    _nrn_double_ptr_pop,
    _nrn_double_ptr_push,
    _nrn_object_ref,
    _nrn_rangevar_get,
    _nrn_rangevar_push,
    _nrn_rangevar_push_nothrow,
    _nrn_rangevar_set,
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
    _ERR_BUF_SIZE,
)

# strdup/free for NrnStrRef. Loaded lazily so import never fails on an
# exotic platform without the default libc name. The strings are freed by
# NEURON (hoc_assign_str) and NEURON's strings by us, so they must come from
# the allocator libnrniv uses: the process's global symbols, which include a
# sanitizer's interceptors (an AddressSanitizer build preloads them; libc's own
# free would abort on their pointers).
_libc = None


def _get_libc():
    global _libc
    if _libc is None:
        import ctypes.util

        try:
            lib = ctypes.CDLL(None)
            lib.strdup
        except (OSError, AttributeError):
            lib = ctypes.CDLL(ctypes.util.find_library("c") or "libc.so.6")
        lib.strdup.argtypes = [ctypes.c_char_p]
        lib.strdup.restype = ctypes.c_void_p
        lib.free.argtypes = [ctypes.c_void_p]
        lib.free.restype = None
        _libc = lib
    return _libc


_NSEG_GENERATION = None


def _nseg_generations():
    # sections imports this module; bind its counter on first use.
    global _NSEG_GENERATION
    if _NSEG_GENERATION is None:
        from .sections import NSEG_GENERATION

        _NSEG_GENERATION = NSEG_GENERATION
    return _NSEG_GENERATION


_sync_nseg = None


def _nseg_generation(sec_ptr):
    generation = _nseg_generations().get(sec_ptr)
    if generation is None:
        from .sections import nseg_generation

        generation = nseg_generation(sec_ptr)
    return generation


def _symbol_info(name):
    from .sections import _get_symbol_info

    return _get_symbol_info(name.decode("utf-8"))


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
        self._nseg = _nseg_generation(sec._sec)

    def _check_nseg(self, sec):
        # Also catches an nseg change made from HOC code (sync_nseg), which
        # NEURON's handle sees: its node may be gone (datahandle test_1).
        global _sync_nseg
        if _sync_nseg is None:
            from .sections import sync_nseg as _sync_nseg
        _sync_nseg(sec._sec)
        generation = _nseg_generations().get(sec._sec, 0)
        if generation == self._nseg:
            return
        from .sections import follow_node

        x = follow_node(sec._sec, self._nseg, self._x, bool(sec.orientation()))
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
        from .api import _nrn_rangevar_handle_str

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

    def _section(self):
        if self._sec is not None:
            return self._sec
        sec = self._sec_ref()
        if sec is None:
            raise ReferenceError("Referenced section no longer exists")
        return sec

    def _push(self):
        sec = self._section()
        # A section deleted through HOC keeps its wrapper but frees the C
        # section; _check_alive raises RuntimeError instead of pushing freed state.
        sec._check_alive()
        self._check_nseg(sec)
        info = _symbol_info(self._name)
        if not info[2]:
            _nrn_rangevar_push(self._sym, sec._sec, self._x)
            return
        # nrn_rangevar_push on an array symbol reads its index from the HOC
        # stack and throws through ctypes (process abort) without it.
        if _nrn_rangevar_push_nothrow is None:
            raise RuntimeError(
                f"a pointer to {self._name.decode('utf-8')}[{self._index or 0}] "
                "needs nrn_rangevar_push_nothrow, which this NEURON lacks (gap-70)"
            )
        err = ctypes.create_string_buffer(_ERR_BUF_SIZE)
        if _nrn_rangevar_push_nothrow(
            self._sym, sec._sec, self._x, self._index or 0, err, _ERR_BUF_SIZE
        ):
            raise RuntimeError(err.value.decode("utf-8", "replace"))

    def _checked_target(self):
        """Return (section, name, kind) or raise before the core is touched.

        nrn_rangevar_get/set dereference unchecked. The core throws through
        ctypes and aborts the process after the mechanism is uninserted, on an
        array symbol, or through an unset POINTER.
        """
        from .sections import Segment, _get_symbol_info, _is_pointer_rangevar

        sec = self._section()
        sec._check_alive()  # HOC-deleted section: RuntimeError, not SIGABRT.
        self._check_nseg(sec)
        name = (
            self._name.decode("utf-8") if isinstance(self._name, bytes) else self._name
        )
        try:
            Segment(sec, self._x)._check_rangevar_mechanism(name)
        except AttributeError:
            raise ValueError("Invalid data_handle") from None
        info = _get_symbol_info(name)
        if info is None:
            raise NameError(f"No such mechanism or variable: {name}")
        if info[2]:
            return sec, name, ("array", info[3])
        if _is_pointer_rangevar(name, info[0]):
            return sec, name, "pointer"
        return sec, name, info[0]

    def _element(self, idx, name, length):
        # NEURON's element pointer indexes relative to itself:
        # seg._ref_xg[0][1] is xg[1], and seg._ref_xg[1][-1] is xg[0].
        if isinstance(idx, bool) or not isinstance(idx, numbers.Integral):
            raise TypeError(f"indices must be integers, not {type(idx).__name__}")
        base = self._index or 0
        element = base + int(idx)
        if not 0 <= element < length:
            raise IndexError(f"{name}[{base}] next_array_element({idx}): out of range")
        return element

    def __getitem__(self, idx):
        sec, name, kind = self._checked_target()
        if isinstance(kind, tuple):  # ("array", length)
            from .sections import _ArrayRangeVar

            length = kind[1]
            element = self._element(idx, name, length)
            return _ArrayRangeVar(sec, self._x, name, length)[element]
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        if kind == "pointer":
            from .sections import _pointer_rangevar_get

            return _pointer_rangevar_get(sec, self._x, name)
        return _nrn_rangevar_get(kind, sec._sec, self._x)

    def __setitem__(self, idx, value):
        sec, name, kind = self._checked_target()
        if isinstance(kind, tuple):  # ("array", length)
            from .sections import _ArrayRangeVar

            length = kind[1]
            element = self._element(idx, name, length)
            _ArrayRangeVar(sec, self._x, name, length)[element] = value
            return
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        if kind == "pointer":
            from .sections import _pointer_rangevar_set

            _pointer_rangevar_set(sec, self._x, name, value)
            return
        _nrn_rangevar_set(kind, sec._sec, self._x, value)


class NrnRangeArrayRef(NrnRef):
    """``seg._ref_<array>`` / ``seg.mech._ref_<array>``: ``[i]`` points at element ``i``.

    Like NEURON's ``nrn.RangeVar``, it has a len() and iterates over the
    element pointers, but is not itself a pointer: ``vec.record(seg._ref_xg)``
    fails where ``vec.record(seg._ref_xg[1])`` records ``xg[1]``.
    """

    __slots__ = ("_sec", "_x", "_name")

    def __init__(self, sec, x, name):
        self._sec = sec
        self._x = x
        self._name = name

    def __len__(self):
        return _symbol_info(self._name.encode("utf-8"))[3]

    def _resolve(self, idx):
        if isinstance(idx, bool) or not isinstance(idx, numbers.Integral):
            raise TypeError(f"indices must be integers, not {type(idx).__name__}")
        length = len(self)
        idx = int(idx)
        if idx < 0:
            idx += length
        if not 0 <= idx < length:
            raise IndexError(self._name)
        return idx

    def __getitem__(self, idx):
        return NrnRangeVarRef(self._sec, self._x, self._name, self._resolve(idx))

    def __setitem__(self, idx, value):
        self[idx][0] = value

    def __iter__(self):
        for i in range(len(self)):
            yield self[i]

    def _push(self):
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
        self._sym = _nrn_symbol(self._name)
        if self._sym is None:
            raise NameError(f"No such mechanism or variable: {self._name}")

    def __repr__(self):
        return f"<NrnRef to n.{self._name.decode()}>"

    def _push(self):
        # Push a raw double* so the HOC function being called (typically
        # Vector.record) can store the address and read it later. The C API
        # has no symbol-pointer push: fetch the address with
        # nrn_symbol_dataptr and push it with double_ptr_push.

        value_ptr = _nrn_symbol_dataptr(self._sym)
        addr = ctypes.cast(value_ptr, ctypes.c_void_p).value if value_ptr else None
        # Bogus small address: see api._MIN_VALID_DATAPTR. Pushing it would
        # SIGSEGV, so fail cleanly.
        if not addr or addr < _MIN_VALID_DATAPTR:
            raise RuntimeError(
                f"cannot take a recordable pointer to '{self._name.decode()}' "
                "on this NEURON build (needs nrn#3815)"
            )
        _nrn_double_ptr_push(value_ptr)

    def __getitem__(self, idx):
        if idx != 0:
            raise IndexError("Only index 0 is supported")

        value_ptr = _nrn_symbol_dataptr(self._sym)
        addr = ctypes.cast(value_ptr, ctypes.c_void_p).value if value_ptr else None
        # Bogus address (see api._MIN_VALID_DATAPTR): n._ref_x[0] is n.x, so
        # read through the guarded top-level path.
        if not addr or addr < _MIN_VALID_DATAPTR:
            from . import NEURON

            return getattr(NEURON(), self._name.decode("utf-8"))
        return ctypes.cast(value_ptr, ctypes.POINTER(ctypes.c_double))[0]

    def __setitem__(self, idx, value):
        if idx != 0:
            raise IndexError("Only index 0 is supported")

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
        # idx=None: scalar property; idx=int: that array element.
        obj = self._obj()
        if obj is None:
            raise ReferenceError("Referenced object no longer exists")
        # The weakref catches a collected wrapper, but a point process whose
        # host section was deleted through HOC keeps its wrapper while the C
        # node is freed, and pushing its property then SIGABRTs. Same guard as
        # the direct ic.amp path.
        obj._check_host_alive()

        if self._idx is None:
            _property_push_checked(obj._obj, self._name)
        else:
            _property_array_push_checked(obj._obj, self._name, int(self._idx))

    def __getitem__(self, idx):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        obj = self._obj()
        if obj is None:
            raise ReferenceError("Referenced object no longer exists")
        obj._check_host_alive()  # HOC-deleted host: clean raise, not SIGABRT.

        if self._idx is None:
            return _property_get_checked(obj._obj, self._name)
        else:
            return _property_array_get_checked(obj._obj, self._name, int(self._idx))

    def __setitem__(self, idx, value):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        obj = self._obj()
        if obj is None:
            raise ReferenceError("Referenced object no longer exists")
        obj._check_host_alive()  # HOC-deleted host: clean raise, not SIGABRT.

        if self._idx is None:
            _ref_property_set_checked(obj._obj, self._name, value)
        else:
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

    def _push(self):
        _nrn_double_ptr_push(self._address())

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
    from .api import _nrn_symbol_array_length, _nrn_symbol_is_array, symbol_array_dims

    sym = _nrn_symbol(name.encode("utf-8"))
    if not sym or not _nrn_symbol_is_array(sym):
        raise TypeError(f"'{name}' is no longer an array")
    current = symbol_array_dims(sym, int(_nrn_symbol_array_length(sym)))
    if current != dims:
        raise TypeError(f"'{name}' was redeclared with dimensions {current}")
    data = _nrn_symbol_dataptr(sym)
    address = ctypes.cast(data, ctypes.c_void_p).value
    if not address or address < _MIN_VALID_DATAPTR:
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
        if isinstance(offset, bool) or not isinstance(offset, numbers.Integral):
            raise TypeError(f"indices must be integers, not {type(offset).__name__}")
        data = _double_array_data(self._name, self._dims)
        total = 1
        for d in self._dims:
            total *= d
        element = self._flat + int(offset)
        if not 0 <= element < total:
            raise IndexError(self._name)
        return ctypes.cast(
            ctypes.addressof(data.contents) + 8 * element,
            ctypes.POINTER(ctypes.c_double),
        )

    def _push(self):
        _nrn_double_ptr_push(self._address())

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
        if isinstance(offset, bool) or not isinstance(offset, numbers.Integral):
            raise TypeError(f"indices must be integers, not {type(offset).__name__}")
        nrow, ncol = int(self._matrix.nrow()), int(self._matrix.ncol())
        flat = self._flat + int(offset)
        if not 0 <= flat < nrow * ncol:
            raise IndexError(f"{self._matrix.hname()}.x")
        return divmod(flat, ncol)

    def _push(self):
        from .api import _nrn_property_element_ptr

        row, col = self._element()
        if _nrn_property_element_ptr is None:
            raise RuntimeError(
                "a pointer to a Matrix element needs nrn_property_element_ptr, "
                "which this NEURON lacks (nrn#3900)"
            )
        indices = (ctypes.c_int * 2)(row, col)
        ptr = ctypes.POINTER(ctypes.c_double)()
        err = ctypes.create_string_buffer(_ERR_BUF_SIZE)
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

    def __init__(self, owner, name, size):
        self._owner = owner
        self._name = name
        self._size = size

    def __len__(self):
        raise TypeError("Most HocObject have no len()")

    def __getitem__(self, idx):
        if not isinstance(idx, int) or isinstance(idx, bool):
            raise TypeError("array reference index must be an integer")
        if idx < 0:
            idx += self._size
        if not 0 <= idx < self._size:
            raise IndexError(
                f"index {idx} out of range [0, {self._size}) for {self._name}"
            )
        return self._owner._property_ref(self._name, idx=idx)

    def __setitem__(self, idx, value):
        self[idx][0] = value

    def _push(self):
        self[0]._push()

    def _pop(self):
        _nrn_double_ptr_pop()

    def __repr__(self):
        return f"<NrnRef to {self._owner.hname()}.{self._name}>"


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

    def _push(self):
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

    def _push(self):
        _nrn_double_ptr_push(
            ctypes.cast(self._address, ctypes.POINTER(ctypes.c_double))
        )

    def __getitem__(self, idx):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        return ctypes.c_double.from_address(self._address).value

    def __setitem__(self, idx, value):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        ctypes.c_double.from_address(self._address).value = float(value)


def numpy_element_ref(numpy_array, index):
    """A HOC reference to element `index` of a contiguous float64 array, as
    neuron.numpy_element_ref. No bounds checking, as in NEURON."""
    address = numpy_array.ctypes.data + int(index) * ctypes.sizeof(ctypes.c_double)
    return NrnRawDoubleRef(address, numpy_array)


class NrnStrRef(NrnRef):
    """Fresh anonymous string cell, like real NEURON's ``h.ref('')``.

    Backs a libc-``strdup`` ``char*`` (a ``c_void_p`` cell) and pushes the
    cell's address as a HOC ``$s1`` string pointer arg (``hoc_pushstr``
    equivalent, ``nrn_str_push(char**)``).

    The cell must be libc-allocated (or NULL). On ``$s1 = "..."``,
    ``hoc_assign_str`` ``free()``s the current ``char*`` and stores an
    ``emalloc``'d (``std::malloc``) copy, so Python-owned ctypes string memory
    would corrupt the heap. See ``nrnpy_hoc.cpp`` ``mkref`` / ``hoc_assign_str``;
    covered by a 50-writeback HOC stress test (``tests/regression/test_hoc_ref.py``
    ``test_str_ref_repeated_hoc_writeback_no_corruption``).
    Python-side ``r[0] = s`` does the same: ``free`` the old, ``strdup`` the
    new. ``__del__`` frees the final buffer.
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
        _nrn_str_push(
            ctypes.cast(ctypes.addressof(self._cell), ctypes.POINTER(ctypes.c_char_p))
        )

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
        libc = _get_libc()
        old = self._cell.value
        self._cell.value = libc.strdup(data)
        if old:
            libc.free(old)

    def __del__(self):
        # Guarded for a half-constructed instance (strdup or _get_libc failed).
        cell = getattr(self, "_cell", None)
        if cell is not None and cell.value:
            try:
                _get_libc().free(cell.value)
            except Exception:
                pass


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

    def _push(self):
        _object_ptr_push(ctypes.byref(self._cell))

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
        from .utils import _wrap_owned_object

        _nrn_object_ref(ptr)
        # Pass the raw int, not c_void_p: Object._obj is an int everywhere
        # (nrn_object_pop's c_void_p restype yields one) and Object.__eq__
        # compares it, so a c_void_p would make a round-tripped object
        # unequal to its source.
        return _wrap_owned_object(ptr)

    def __setitem__(self, idx, value):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        from .api import _nrn_object_unref
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
        # Release the cell's one reference. Guarded like Object.__del__ against
        # interpreter teardown and half-constructed instances.
        if (
            getattr(sys, "meta_path", None) is None
            or getattr(sys, "modules", None) is None
        ):
            return
        cell = getattr(self, "_cell", None)
        if cell is None or not cell.value:
            return
        try:
            from .api import _nrn_object_unref

            _nrn_object_unref(cell.value)
        except Exception:
            pass
