"""Object, List, Vector wrappers — dynamic class system, refcounting, method dispatch."""
import ctypes
import sys
import warnings

from ._docs import _ClassDoc


# ---------------------------------------------------------------------------
# Metaclass — _HocClassMeta
# ---------------------------------------------------------------------------


class _HocClassMeta(type):
    """Metaclass for HOC-wrapper classes.

    Provides ``Cls[i]`` indexing into the live HOC List of instances of
    the given class. ``n.IClamp[0]`` returns the first IClamp in the
    model; ``n.IClamp[-1]`` the most recent one.

    Putting this on a metaclass (rather than wrapping the class in a
    proxy) is what lets ``n.IClamp`` itself be the dynamic class:
    callable via ``type.__call__`` → ``Object.__init__``, and
    subclassable in plain Python (``class MyIClamp(n.IClamp): ...``)
    with no ``neuron.hclass()`` wrapper.
    """

    def __new__(mcls, name, bases, namespace, **kwargs):
        hoc_name = namespace.get('_hoc_class_name')
        if hoc_name:
            namespace['__doc__'] = _ClassDoc(hoc_name, namespace.get('__doc__'))
        return super().__new__(mcls, name, bases, namespace, **kwargs)

    @property
    def __doc__(cls):
        # pydoc uses object.__getattribute__ on classes, bypassing the class's
        # own descriptor lookup. A metaclass property serves that path too.
        doc = vars(cls).get('__doc__')
        return doc.__get__(None, cls) if isinstance(doc, _ClassDoc) else doc

    @__doc__.setter
    def __doc__(cls, value):
        # Bypass this property's setter while updating the actual class dict,
        # so an explicit override is also visible on instances.
        type.__dict__['__doc__'].__set__(cls, value)

    def __getitem__(cls, index):
        # _hoc_class_name is set on every dynamic class minted by
        # NEURON.__getattr__; the hand-coded subclasses (List, Vector,
        # SectionList) set it explicitly below. Base Object itself is
        # not indexable — it has no HOC counterpart.
        class_name = getattr(cls, "_hoc_class_name", None)
        if class_name is None:
            raise TypeError(
                f"class {cls.__name__!r} cannot be indexed — no HOC class bound"
            )
        from . import NEURON

        # HOC's `List("ClassName")` enumerates every existing instance,
        # ordered by allocation (which doesn't match object_id order).
        # Negative indices are resolved by sorting object_ids; positive
        # indices match by object_id, mirroring real NEURON's
        # `h.IClamp[i]` semantics.
        n = NEURON()
        lst = n.List(class_name)
        count = int(lst.count())
        if count == 0:
            raise IndexError(f"{class_name}[{index}]: no instances exist")
        if index < 0:
            ids = []
            for i in range(count):
                obj = lst.object(i)
                ids.append((int(n.object_id(obj, 1)), i))
            ids.sort()
            if -index > len(ids):
                raise IndexError(f"{class_name}[{index}] out of range")
            _, list_idx = ids[index]
            return lst.object(list_idx)
        # O(n) scan — object_ids are not contiguous positions.
        for i in range(count):
            obj = lst.object(i)
            if int(n.object_id(obj, 1)) == index:
                return obj
        raise IndexError(f"{class_name}[{index}] not found")


def _construct_hoc_object(class_name, args, sec=None):
    """Create the HOC object backing a user-side construction call.

    Returns ``(obj_ptr, host_section, post_init, abort_init)``. Callback
    registrations remain owned by this construction transaction until
    ``post_init`` hands them to the finished wrapper; ``abort_init`` releases
    them if wrapper initialization fails before that handoff.

    Three arg-rewrite cases (gap-3): Vector iterable, SectionList
    iterable, FInitializeHandler callable — all have Python convenience
    signatures that don't match the HOC C constructor. Everything else
    passes args straight through.

    Called from ``Object.__init__`` when the user constructs a dynamic
    class directly (``n.IClamp(seg)`` or ``MyIClamp(seg)`` for any
    Python subclass). The internal raw-pointer path goes through
    ``Object._wrap`` instead and doesn't touch this function.
    """
    import collections.abc
    from .api import (
        _nrn_symbol,
        _nrn_object_new,
        _nrn_section_push,
        _nrn_section_pop,
    )
    from .utils import _push_args

    # Three HOC classes accept Python idioms that don't map to a
    # straight HOC constructor call. Rewrite the args so the HOC side
    # gets what its C constructor expects, then finish the rest from
    # Python after the wrapper exists.
    initing_to_list_arg = None
    initing_sectionlist = None
    fih_callback_ids = []
    args_for_push = args

    def cleanup_fih_callbacks():
        if not fih_callback_ids:
            return
        from . import _unregister_py_callback

        for cb_id in fih_callback_ids:
            _unregister_py_callback(cb_id)
        fih_callback_ids.clear()

    if (
        class_name == "Vector"
        and len(args) == 1
        and isinstance(args[0], collections.abc.Iterable)
        and not isinstance(args[0], str)
    ):
        # n.Vector([1, 2, 3]) → HOC Vector(3) + later fill from Python.
        initing_to_list_arg = list(args[0])
        args_for_push = (len(initing_to_list_arg),)
    elif (
        class_name == "SectionList"
        and len(args) == 1
        and isinstance(args[0], collections.abc.Iterable)
        and not isinstance(args[0], str)
    ):
        # n.SectionList([s1, s2]) → empty HOC SectionList + later append().
        initing_sectionlist = list(args[0])
        args_for_push = ()
    elif (
        class_name == "FInitializeHandler"
        and len(args) >= 1
        and any(callable(a) and not isinstance(a, str) for a in args)
    ):
        # Rewrite the Python callable into a HOC command string
        # that hits our `_mn_py_callback(id)` trampoline. The id keeps
        # the callable alive in `_PY_CALLBACKS` until the FIH wrapper
        # drops its `_fih_callback_id` slot in __del__.
        from . import _register_py_callback

        new_args = list(args)
        try:
            for i, a in enumerate(new_args):
                if callable(a) and not isinstance(a, str):
                    cb_id = _register_py_callback(a)
                    fih_callback_ids.append(cb_id)
                    new_args[i] = f"_mn_py_callback({cb_id})"
        except BaseException:
            cleanup_fih_callbacks()
            raise
        args_for_push = tuple(new_args)

    # Liveness guard BEFORE any push: constructing a point process
    # on a section deleted through HOC (e.g. n.IClamp(dead_seg)) otherwise
    # pushed the dead section straight into nrn_object_new and SIGABRTed on the
    # first call. Guard the explicit sec= here; a Segment arg's section is
    # checked inside _push_args.
    try:
        if sec is not None:
            sec._check_alive()
        temp_strs, seg_sec, rollback_args = _push_args(
            args_for_push, return_rollback=True
        )
    except BaseException:
        cleanup_fih_callbacks()
        raise
    # `sec=` kwarg wins over a section implied by a Segment arg.
    use_sec = sec if sec is not None else seg_sec
    if use_sec is not None:
        _nrn_section_push(use_sec._sec)
    try:
        sym = _nrn_symbol(class_name.encode("utf-8"))
        obj = _nrn_object_new(sym, len(args_for_push))
    except BaseException:
        # Construction failed after a FInitializeHandler callable was registered
        # (its id is embedded in the pushed args as _mn_py_callback(id)). No
        # wrapper exists to drop it in __del__, so drop it here or it leaks in
        # _PY_CALLBACKS forever.
        cleanup_fih_callbacks()
        # nrn_object_new_nothrow restores the caller-owned argument stack when
        # construction fails. Remove those values before surfacing the error so
        # repeated failures cannot accumulate into "Stack too deep".
        rollback_args()
        from . import _raise_pending_callback_exc
        _raise_pending_callback_exc()
        raise
    finally:
        if use_sec is not None:
            _nrn_section_pop()

    def post_init(wrapper):
        if fih_callback_ids:
            # FInitializeHandler's supported signature has one callable. Keep
            # the public/internal scalar used by existing callers and tests.
            wrapper._fih_callback_id = fih_callback_ids[0]
            from . import _unregister_py_callback

            for cb_id in fih_callback_ids[1:]:
                _unregister_py_callback(cb_id)
            fih_callback_ids.clear()
        if initing_to_list_arg is not None:
            for i, data in enumerate(initing_to_list_arg):
                wrapper[i] = data
        if initing_sectionlist is not None:
            for s in initing_sectionlist:
                wrapper.append(sec=s)

    return obj, use_sec, post_init, cleanup_fih_callbacks


# ---------------------------------------------------------------------------
# Base wrapper class
# ---------------------------------------------------------------------------


class Object(metaclass=_HocClassMeta):
    """Wraps a HOC object (template instance) — IClamp, Vector, NetCon, etc.

    Subclasses are minted by ``NEURON.__getattr__`` the first time a HOC
    template is accessed via ``n.<ClassName>``; the user-facing class object
    IS the dynamic subclass. Construct instances by calling the class
    (``n.IClamp(soma(0.5))``); pure-Python subclassing is supported
    (``class MyClamp(n.IClamp): ...``). Attribute access proxies through
    HOC template-member dispatch via ``__getattr__`` / ``__setattr__``.
    """

    # __weakref__ slot lets Vector / NetCon / etc. be the target of
    # weakref.ref (some test patterns rely on this). _section_cache and
    # _fih_callback_id are populated lazily — declared up front so the
    # slot descriptors exist before any read attempt.
    __slots__ = (
        "_obj",
        "_class_name",
        "_methods",
        "_host_section",
        "_host_section_ref",
        "_section_cache",
        "_fih_callback_id",
        "__weakref__",
    )

    # Set on dynamic subclasses by NEURON.__getattr__; the hand-coded
    # subclasses below set it explicitly. Drives both metaclass indexing
    # (``cls[i]``) and the user-construction path in __init__.
    _hoc_class_name = None

    def __init__(self, *args, host_section=None, sec=None):
        # Two construction paths share one __init__:
        #
        #   (a) internal — raw HOC obj* (a ctypes.c_void_p) passed in by
        #       _nrn_object_new callers or Object._wrap. Path used by
        #       _object_pop, FuncWrapper return marshalling, and the
        #       NEURON.__getattr__ TEMPLATE branch.
        #
        #   (b) user — the dynamic class is called directly with HOC
        #       constructor args (``n.IClamp(seg)``, ``MyCell()`` after
        #       subclassing). The class's bound _hoc_class_name names the
        #       HOC template, _construct_hoc_object handles arg pushing
        #       and the iterable / FIH special cases.
        #
        # An explicit ctypes.c_void_p disambiguates the two cleanly; user
        # code never passes a raw c_void_p as a HOC constructor arg.
        if (
            len(args) == 1
            and isinstance(args[0], ctypes.c_void_p)
            and sec is None
        ):
            obj = args[0]
            post_init = None
        else:
            class_name = type(self)._hoc_class_name
            if class_name is None:
                raise TypeError(
                    f"{type(self).__name__} has no bound HOC class — "
                    "instantiate via n.<ClassName>(...) or subclass a "
                    "class returned by n.<ClassName>."
                )
            obj, host_from_args, post_init, abort_init = _construct_hoc_object(
                class_name, args, sec=sec
            )
            if host_section is None:
                host_section = host_from_args

        try:
            self._init_from_ptr(obj, host_section)
            if post_init is not None:
                post_init(self)
        except BaseException:
            if post_init is not None:
                abort_init()
            raise

    @classmethod
    def _wrap(cls, obj, host_section=None):
        """Wrap an existing HOC obj* without invoking the user
        construction path.

        Used by internal callers that already have a raw pointer
        (NEURON.__getattr__ template branch, _object_pop). Bypasses any
        user-defined __init__ on a subclass, since we just want the
        Python wrapper around an existing HOC object.
        """
        instance = cls.__new__(cls)
        # Establish ownership before initialization can fail. A partially
        # initialized wrapper then releases the caller-provided ref in __del__.
        instance._obj = obj
        instance._init_from_ptr(obj, host_section)
        return instance

    def _init_from_ptr(self, obj, host_section):
        """Populate all slots from an existing HOC obj*.

        Called by both ``__init__`` (user path) and ``_wrap`` (internal path).
        Takes refcount ownership of obj — the caller must NOT separately call
        nrn_object_ref; ``__del__`` will call nrn_object_unref.
        """
        from .api import _nrn_class_name

        self._obj = obj
        # The caller (nrn_object_new or nrn_object_pop) has already
        # provided a reference. We take ownership and release it in __del__.
        self._class_name = _nrn_class_name(obj).decode("utf-8")
        from .utils import list_methods

        self._methods = list_methods(self)
        # A HOC/template-owned Section wrapper is non-owning and may be a
        # temporary (cell.soma[0]). Keep that wrapper alive with its dependent;
        # the template owns the native Section, so wrapper GC says nothing
        # about native liveness (gap-50). Python-owned Sections keep the weak
        # hold required by real NEURON's last-wrapper-deletes-section contract.
        self._host_section = None
        self._host_section_ref = None
        if host_section is not None and host_section.is_pysec():
            import weakref

            self._host_section_ref = weakref.ref(host_section)
        elif host_section is not None:
            self._host_section = host_section
        # Lazy slots: written by __getattr__ (template section cache) and
        # by the FIH dispatch path. Initialised to None so reads hit a
        # valid value instead of cascading into __getattr__ via the
        # unset-slot AttributeError.
        self._section_cache = None
        self._fih_callback_id = None

    def __del__(self):
        # nrn_object_new returns refcount 1; we DO NOT call nrn_object_ref in
        # __init__ because that would double-ref and leak the C object.
        # Python shutdown guard: meta_path/modules can be None during interpreter
        # teardown, in which case importing from .api would crash.
        if (
            getattr(sys, "meta_path", None) is None
            or getattr(sys, "modules", None) is None
        ):
            return
        # Note: libnrniv may be partially torn down before all Objects are
        # collected (arbitrary finalizer order). The meta_path/modules guard
        # above catches the common case; pathological atexit patterns may still
        # reach _nrn_object_unref after the library is gone.
        # Release any Python callback held by this Object (FInitializeHandler
        # with a Python callable — gap-3). A partially-initialised Object
        # may not have any slots set, so wrap each read in try/except —
        # unset __slots__ entries raise AttributeError on access.
        try:
            cb_id = self._fih_callback_id
        except AttributeError:
            cb_id = None
        if cb_id is not None:
            try:
                from . import _unregister_py_callback
                _unregister_py_callback(cb_id)
            except Exception:
                pass
        try:
            obj = self._obj
        except AttributeError:
            obj = None
        if obj is not None:
            from .api import _hoc_unref_defer, _nrn_object_unref

            _nrn_object_unref(obj)
            # Prompt deletion, as when NEURON releases a HocObject wrapper.
            _hoc_unref_defer()

    def hname(self):
        from . import NEURON

        idx = int(NEURON().object_id(self, 1))
        return f"{self._class_name}[{idx}]"

    def baseattr(self, name):
        """Look up *name* on the HOC side, ignoring Python-level overrides.

        Real NEURON's `HocObject.baseattr(name)` is literally
        `hocobj_getattr(self, name)` (nrnpy_hoc.cpp:1410-1416) — the
        whole point is bypassing any Python subclass that shadows a
        HOC attribute with a `@property`, descriptor, or instance
        `__dict__` entry. In myneuron the HOC dispatch lives in
        `Object.__getattr__`, which Python only consults when normal
        attribute lookup misses. We call it directly to force the HOC
        path. Using `Object.__getattr__` (not `type(self).__getattr__`)
        means subclasses can override `__getattr__` for non-HOC reasons
        without breaking baseattr's contract.
        """
        return Object.__getattr__(self, name)

    def __repr__(self):
        return self.hname()

    def __str__(self):
        return self.hname()

    def same(self, other):
        """Return True if *other* wraps the same underlying HOC object."""
        if not isinstance(other, Object):
            return False
        return self._obj == other._obj

    def __eq__(self, other):
        # Two Python wrappers for the same HOC object compare equal.
        # Matches real NEURON's hocobj tp_richcompare (nrnpy_hoc.cpp):
        # round-tripping a Vector through a List yields a NEW wrapper
        # with the same underlying obj*. Non-Object operands return
        # NotImplemented so Python falls back to the other side's
        # __eq__ (and then to False with no exception).
        if not isinstance(other, Object):
            return NotImplemented
        return self._obj == other._obj

    def __ne__(self, other):
        result = self.__eq__(other)
        if result is NotImplemented:
            return result
        return not result

    def __hash__(self):
        # Must stay consistent with __eq__: same pointer => same hash.
        return hash(self._obj) if self._obj else 0

    def __copy__(self):
        """Reject copying HOC objects other than Vector.

        Real NEURON only exposes Python's copy protocol for Vector.  Letting
        ``copy.copy`` fall back to the wrapper's slots would duplicate the
        owned native pointer without taking a native reference, so the two
        wrappers would race to unref the same object.
        """
        raise TypeError("HocObject: Only Vector instance can be pickled")

    def __deepcopy__(self, memo):
        """Reject deepcopy for HOC objects other than Vector."""
        raise TypeError("HocObject: Only Vector instance can be pickled")

    def hocobjptr(self):
        """Return the raw C pointer to the underlying HOC object as an int."""
        return int(self._obj) if self._obj is not None else 0

    def ref(self, name, idx=None):
        from .nrnref import NrnHocTemplateVarRef, NrnObjectPropertyRef
        from .api import TYPES

        methods = getattr(self, "_methods", None)
        if methods is not None and name in methods and methods[name][0] == TYPES.VAR:
            from .api import _nrn_method_symbol, _nrn_symbol_is_array

            sym = _nrn_method_symbol(self._obj, name.encode("utf-8"))
            is_array = bool(_nrn_symbol_is_array(sym))
            if is_array and idx is None:
                raise TypeError(
                    f"array property '{name}' requires an explicit index: "
                    f"obj.ref('{name}', idx=i)"
                )
            if not is_array and idx is not None:
                raise TypeError(f"scalar property '{name}' does not take an index")
            return NrnHocTemplateVarRef(self, name, idx=idx)
        # Real class property (ic._ref_amp), an array element, or no method
        # table to validate against -> native property-pointer ref.
        if methods is None or name in methods or idx is not None:
            return NrnObjectPropertyRef(self, name, idx=idx)
        # Not a class property. A StringFunctions alias resolves via trampolines;
        # anything else raises a clean AttributeError rather than a
        # NrnObjectPropertyRef that would SIGSEGV in _nrn_property_get on a
        # non-member name.
        alias = _alias_ref(self, name)
        if alias is not None:
            return alias
        raise AttributeError(
            f"'{self._class_name}' object has no property or alias '{name}' to reference"
        )

    def __dir__(self):
        # `del` is a valid HOC field name (IClamp.del, AlphaSynapse.del)
        # but a Python keyword; expose it as `delay` in dir() and dispatch.
        methods = set(self._methods)
        if "del" in methods:
            methods.discard("del")
            methods.add("delay")
        # Surface the Python-side helpers (hname, ref, same, hocobjptr) so
        # users see them in dir(ic). Subclasses (Vector, List, SectionList)
        # contribute their own __dict__ entries too.
        for klass in type(self).__mro__:
            if klass is object:
                continue
            for name in klass.__dict__:
                if not name.startswith("_"):
                    methods.add(name)
        return sorted(methods)

    def _check_host_alive(self):
        """Raise if this point process's host section has been deleted.

        No-op for non-point-process objects (both host slots are None).

        Python-owned Sections are weakly held so their last owning wrapper can
        delete the native section; a stale weakref raises ``ReferenceError``.
        HOC/template-owned wrappers are strongly held because their native
        Section has an independent owner. In either case an explicit native
        ``delete_section`` is caught by ``Section._check_alive`` and raises
        ``RuntimeError`` before property dispatch can dereference freed state.
        """
        host = self._host_section
        if host is None:
            ref = self._host_section_ref
            if ref is None:
                return
            host = ref()
            if host is None:
                raise ReferenceError(
                    f"{self._class_name}: host section has been deleted"
                )
        host._check_alive()

    def _assign_pointer(self, member, src_ref):
        """Wire this object's NMODL POINTER ``member`` to the address referenced
        by ``src_ref`` (an NrnRef), implementing ``obj._ref_PTR = src._ref_X``.

        The public ``nrn_pp_setpointer_pop`` API addresses the point process
        directly and consumes exactly one source handle.
        """
        from .api import _nrn_pp_setpointer_pop, _ERR_BUF_SIZE

        member_encoded = member.encode("utf-8")
        err = ctypes.create_string_buffer(_ERR_BUF_SIZE)
        src_ref._push()
        rc = _nrn_pp_setpointer_pop(
            self._obj, member_encoded, err, _ERR_BUF_SIZE
        )
        if rc:
            msg = err.value.decode("utf-8", errors="replace")
            raise RuntimeError(msg or f"setpointer {member} failed")

    def __setattr__(self, name, val):
        # POINTER assignment: obj._ref_PTR = src._ref_X wires an NMODL POINTER.
        # Must come before the generic underscore short-circuit below.
        if name.startswith("_ref_"):
            from .nrnref import NrnRef

            if isinstance(val, NrnRef):
                self._check_host_alive()
                self._assign_pointer(name[5:], val)
                return
        # Allow setting private attributes (underscore-prefixed) directly —
        # they're internal bookkeeping (host section weakref, FIH callback
        # id, etc.) and never a HOC property name.
        if name.startswith("_"):
            super().__setattr__(name, val)
            return
        # Subclass opted into per-instance __dict__: allow ad-hoc Python
        # attributes (a.bp = ..., a.__doc__ = ...) as long as the name
        # isn't a real HOC property. Without this, custom subclasses
        # can't carry Python state without re-using a HOC property name.
        if "__dict__" in getattr(type(self), "__dict__", {}):
            methods = getattr(self, "_methods", None)
            if methods is None or name not in methods:
                object.__setattr__(self, name, val)
                return
        self._check_host_alive()

        # `delay` → `del` alias — see __dir__ comment above.
        actual_name = name
        if (
            hasattr(self, "_methods")
            and name not in self._methods
            and name == "delay"
            and "del" in self._methods
        ):
            actual_name = "del"

        # Allow setting scalar object properties (RANGEVAR or VAR doubles).
        # A user HOC template's VAR lives in its object dataspace, not behind
        # the C++ property `steer` callback used by nrn_property_set.
        if hasattr(self, "_methods") and actual_name in self._methods:
            _type, _ = self._methods[actual_name]
            from .api import TYPES
            if _type == TYPES.STRING:
                _write_hoc_template_string(self, actual_name, val)
                return
            if _type in (TYPES.RANGEVAR, TYPES.VAR):
                from .api import (
                    _nrn_method_symbol,
                    _nrn_symbol_is_array,
                    _nrn_symbol_array_length,
                    _property_set_checked,
                    _property_array_set_checked,
                )

                name_encoded = actual_name.encode("utf-8")
                sym = _nrn_method_symbol(self._obj, name_encoded)
                if not _nrn_symbol_is_array(sym):
                    if _type == TYPES.VAR:
                        _write_hoc_template_var(self, actual_name, val)
                    else:
                        _property_set_checked(self._obj, name_encoded, val)
                    return
                # Array property: accept a list/tuple of the right length
                # and set each element; matches the MATLAB interface's
                # array-property setter.
                size = _nrn_symbol_array_length(sym)
                try:
                    seq = list(val)
                except TypeError:
                    raise TypeError(
                        f"Array property '{name}' (length {size}) requires a sequence, "
                        f"got {type(val).__name__}"
                    ) from None
                if len(seq) != size:
                    raise ValueError(
                        f"Array property '{name}' has length {size}; "
                        f"got {len(seq)} values"
                    )
                for i, v in enumerate(seq):
                    if _type == TYPES.VAR:
                        _write_hoc_template_var(
                            self, actual_name, float(v), idx=i
                        )
                    else:
                        _property_array_set_checked(
                            self._obj, name_encoded, i, float(v)
                        )
                return
            if TYPES.RANGEOBJ is None or _type == TYPES.RANGEOBJ:
                from .api import _nrn_method_symbol
                from .utils import _try_wrap_point_nmodlrandom

                sym = _nrn_method_symbol(self._obj, actual_name.encode("utf-8"))
                wrapped = _try_wrap_point_nmodlrandom(self._obj, sym, _type)
                if wrapped is not None:
                    raise TypeError(
                        f"NMODL RANDOM variable '{actual_name}' is not assignable"
                    )
        # StringFunctions alias write (obj.aliasname = v) — see __getattr__.
        if (
            name.isidentifier()
            and not name.startswith("_")
            and _write_object_alias(self, name, val)
        ):
            return
        raise AttributeError(
            f"Cannot set '{name}': not a settable HOC property (RANGEVAR/VAR) of {self._class_name}"
        )

    def __getattr__(self, name):
        # If __init__ never set _methods (e.g. a subclass overrode
        # __init__ and didn't call super().__init__), bail before we
        # reach `self._methods` below — otherwise the unset-slot
        # AttributeError would re-enter __getattr__ and recurse.
        try:
            methods = object.__getattribute__(self, "_methods")
        except AttributeError:
            raise AttributeError(name) from None
        # Skip the liveness check for dunder/private names so the
        # weakref bookkeeping itself doesn't recurse.
        if not name.startswith("_"):
            self._check_host_alive()
        if name.startswith("_ref_"):
            return self.ref(name[5:])
        # .plot() on RangeVarPlot / PlotShape — not a HOC method
        if name == "plot":
            if self._class_name == "RangeVarPlot":
                from .plotting import _RangeVarPlot

                return _RangeVarPlot(self)
            elif self._class_name == "PlotShape":
                from .plotting import _PlotShapePlot
                from . import NEURON

                NEURON().define_shape()
                return _PlotShapePlot(self)
        # `delay` → `del` alias — see __dir__ comment above.
        original_name = name
        if name not in self._methods and name == "delay" and "del" in self._methods:
            name = "del"
        if name in self._methods:
            _type, _ = self._methods[name]
            qualified_name = f"{self._class_name}.{original_name}"
            from .api import TYPES

            # A point-process-only MOD may be the first RANGEOBJ encountered,
            # so there is no density symbol from which to learn the runtime
            # code. Probe an unknown component through the fail-closed adapter;
            # ordinary methods pay nothing once RANGEOBJ has been discovered.
            if TYPES.RANGEOBJ is None:
                from .api import _nrn_method_symbol
                from .utils import _try_wrap_point_nmodlrandom

                sym = _nrn_method_symbol(self._obj, name.encode("utf-8"))
                wrapped = _try_wrap_point_nmodlrandom(self._obj, sym, _type)
                if wrapped is not None:
                    return wrapped

            # --- type SECTION: template public section (cell.soma, cell.dend[i]) ---
            if _type == TYPES.SECTION:
                # Not dispatchable via nrn_method_call — the symbol is parsed
                # as a section reference, not a function call. A tiny HOC
                # fragment pushes the section onto the stack; _mn_capture_cas
                # snapshots the pointer from inside the braces block.
                #
                # Per-Object Section* cache (_section_cache): real NEURON uses
                # bytecode-level component() (nrnpy_hoc.cpp:1270), which is
                # not in neuronapi.h. Caching avoids a HOC parse round-trip on
                # every repeated access. Sections live as long as the owning
                # Object, so the cached pointer stays valid indefinitely.
                # Independent caches per instance via the lazily-filled
                # _section_cache slot.
                sec_cache = self._section_cache
                if sec_cache is not None:
                    cached = sec_cache.get(name)
                    if cached is not None:
                        return cached
                from .api import _nrn_method_symbol, _nrn_symbol_is_array

                sym = _nrn_method_symbol(self._obj, name.encode("utf-8"))
                if _nrn_symbol_is_array(sym):
                    # `create dend[N]` — return a subscriptable proxy. Real
                    # NEURON returns a HocObject array; we return a thin
                    # wrapper that resolves each index via the same callback.
                    from .api import _nrn_symbol_array_length

                    size = int(_nrn_symbol_array_length(sym))
                    proxy = _TemplateSectionArray(self, name, size)
                    if sec_cache is None:
                        sec_cache = {}
                        object.__setattr__(self, "_section_cache", sec_cache)
                    sec_cache[name] = proxy
                    return proxy
                resolved = _resolve_template_section(self, name, idx=None)
                if sec_cache is None:
                    sec_cache = {}
                    object.__setattr__(self, "_section_cache", sec_cache)
                sec_cache[name] = resolved
                return resolved

            # --- type OBJECTVAR: template objref member (cell.all) ---
            if _type == TYPES.OBJECTVAR:
                # Not dispatchable via nrn_method_call — the symbol is an
                # object reference, not a method, so the generic method path
                # below would mis-read it (returns a stray FuncWrapper and
                # warns "unknown return type 271"). Read it through a
                # per-member obfunc trampoline; see _read_object_member.
                # Import3d cells expose their SectionLists this way
                # (cell.all / somatic / apical / basal / axonal).
                from .api import (
                    _nrn_method_symbol,
                    _nrn_symbol_is_array,
                    _nrn_symbol_array_length,
                )

                sym = _nrn_method_symbol(self._obj, name.encode("utf-8"))
                if _nrn_symbol_is_array(sym):
                    return _ObjectMemberArray(
                        self, name, int(_nrn_symbol_array_length(sym))
                    )
                return _read_object_member(self, name)

            # --- type STRING: user-template strdef member (obj.label) ---
            if _type == TYPES.STRING:
                return _read_hoc_template_string(self, name)

            # --- type RANGEVAR / VAR: scalar or array property (amp, tau1) ---
            if _type in (TYPES.RANGEVAR, TYPES.VAR):
                from .api import (
                    _nrn_method_symbol,
                    _nrn_symbol_is_array,
                    _nrn_symbol_array_length,
                    _property_get_checked,
                )

                name_encoded = name.encode("utf-8")
                sym = _nrn_method_symbol(self._obj, name_encoded)
                if not _nrn_symbol_is_array(sym):
                    if _type == TYPES.VAR:
                        return _read_hoc_template_var(self, name)
                    return _property_get_checked(self._obj, name_encoded)
                else:
                    return ArrayProperty(
                        self, name, int(_nrn_symbol_array_length(sym)),
                        qualified_name=qualified_name,
                        hoc_template_var=(_type == TYPES.VAR),
                    )

            if TYPES.RANGEOBJ is not None and _type == TYPES.RANGEOBJ:
                from .api import _nrn_method_symbol
                from .utils import _try_wrap_point_nmodlrandom

                sym = _nrn_method_symbol(self._obj, name.encode("utf-8"))
                wrapped = _try_wrap_point_nmodlrandom(self._obj, sym, _type)
                if wrapped is not None:
                    return wrapped

            # --- all other callable types: FUNCTION, METHOD_SECTIONREF,
            #     METHOD_OBFUNC/OBFUNCTION, METHOD_STRFUNC ---
            else:

                def method_func(*args, sec=None):
                    import ctypes
                    from .api import (
                        _nrn_method_symbol,
                        _nrn_method_call,
                        _nrn_double_pop,
                        _nrn_str_pop,
                        _nrn_object_pop,
                        _nrn_section_push,
                        _nrn_section_pop,
                        _check_nrn_error,
                        _ERR_BUF_SIZE,
                        _hoc_return_type_code,
                        _object_is_builtin_class,
                    )
                    from .utils import FuncWrapper, _push_args, _object_pop

                    if _type == TYPES.METHOD_SECTIONREF:
                        # SECTIONREF steering (sec/parent/root/trueparent/child)
                        # resolves correctly only through the HOC brace form; the
                        # bare method-call path returns the tree ROOT for every
                        # one of them. Route to _steer_sectionref BEFORE pushing
                        # args (so child(i)'s index isn't orphaned on the stack).
                        idx = int(args[0]) if (name == "child" and args) else None
                        return _steer_sectionref(self, name, idx)

                    sym = _nrn_method_symbol(self._obj, name.encode("utf-8"))
                    # Liveness guard BEFORE pushing args: an
                    # explicit sec= here, a Segment arg's section inside
                    # _push_args — so a dead-section call raises before any
                    # operand is pushed, never orphaning HOC-stack slots.
                    if sec is not None:
                        sec._check_alive()
                    temp_strs, seg_sec, rollback_args = _push_args(
                        args, return_rollback=True
                    )
                    use_sec = sec if sec is not None else seg_sec
                    if use_sec is not None:
                        _nrn_section_push(use_sec._sec)
                    try:
                        _err_buf = ctypes.create_string_buffer(_ERR_BUF_SIZE)
                        # Reset the return-type hint so a value left by an
                        # earlier call can't leak into this one (nrnpy fcall
                        # does the same before the call).
                        _hoc_return_type_code.value = 0
                        ret = _nrn_method_call(self._obj, sym, len(args), _err_buf, _ERR_BUF_SIZE)
                        from . import _bump_model_epoch

                        _bump_model_epoch()
                        if ret:
                            # Like function calls, failed nothrow method calls
                            # restore caller arguments, not consume them (gap-56).
                            rollback_args()
                        _check_nrn_error(ret, _err_buf)
                        if _type == TYPES.FUNCTION:
                            val = _nrn_double_pop()
                            # A built-in method returning int/bool set the hint.
                            # Trust it only for built-in classes; then reset.
                            code = _hoc_return_type_code.value
                            _hoc_return_type_code.value = 0
                            if code and _object_is_builtin_class(self._obj):
                                if code == 1:
                                    result = int(val)
                                elif code == 2:
                                    result = bool(val)
                                else:
                                    result = val
                            else:
                                result = val
                        elif _type == TYPES.METHOD_OBFUNC or _type == TYPES.OBFUNCTION:
                            result = _object_pop()
                        elif _type == TYPES.METHOD_STRFUNC:
                            result = _nrn_str_pop()
                        else:
                            warnings.warn(f"Ignoring unknown return type {_type}")
                            result = None
                        # A method that fires a Python callback (e.g. a template
                        # method calling finitialize() with a raising
                        # FInitializeHandler) must surface that exception here, at
                        # the method call — not on the next unrelated dispatch.
                        # Do this after popping the HOC return value
                        # so repeated failed HOC->Python callbacks do not leak
                        # native operand-stack slots.
                        from . import _raise_pending_callback_exc

                        _raise_pending_callback_exc()
                        return result
                    finally:
                        if use_sec is not None:
                            _nrn_section_pop()
                        from .api import _hoc_unref_defer

                        _hoc_unref_defer()

                from .utils import FuncWrapper

                return FuncWrapper(
                    method_func,
                    qualified_name,
                    f"NEURON object method: {self._class_name}.{name}()",
                    doc_key=f"{self._class_name}.{name}",
                )
        # StringFunctions alias (obj.aliasname): not in the class symbol table,
        # so the resolution above misses. Resolve via a HOC trampoline — the
        # native property accessors SIGSEGV on a non-member name. Only plain
        # identifiers; dunders/private probes fall straight through.
        if name.isidentifier() and not name.startswith("_"):
            try:
                return _read_object_alias(self, name)
            except AttributeError:
                pass
        raise AttributeError(f"'{self._class_name}' object has no attribute '{name}'")


# User HOC-template numeric members. nrn_property_get/set address C++ template
# properties through ctemplate->steer; pure HOC templates have object dataspace
# instead and no steer callback. Route their VAR components through HOC itself,
# matching the interpreter path used by real NEURON's Python binding.
_HOC_TEMPLATE_VAR_TRAMPOLINES = set()


def _ensure_hoc_template_var_trampolines(name, array):
    key = (name, bool(array))
    if key in _HOC_TEMPLATE_VAR_TRAMPOLINES:
        return
    from . import NEURON

    n = NEURON()
    kind = "a" if array else "s"
    get_name = f"_mn_hvget_{kind}_{name}"
    set_name = f"_mn_hvset_{kind}_{name}"
    if array:
        n(f"func {get_name}() {{ return $o1.{name}[$2] }}")
        n(f"proc {set_name}() {{ $o1.{name}[$2] = $3 }}")
    else:
        n(f"func {get_name}() {{ return $o1.{name} }}")
        n(f"proc {set_name}() {{ $o1.{name} = $2 }}")
    _HOC_TEMPLATE_VAR_TRAMPOLINES.add(key)


def _read_hoc_template_var(owner, name, idx=None):
    from . import NEURON

    array = idx is not None
    _ensure_hoc_template_var_trampolines(name, array)
    fn = f"_mn_hvget_{'a' if array else 's'}_{name}"
    args = (owner, idx) if array else (owner,)
    return getattr(NEURON(), fn)(*args)


def _write_hoc_template_var(owner, name, value, idx=None):
    from . import NEURON

    array = idx is not None
    _ensure_hoc_template_var_trampolines(name, array)
    fn = f"_mn_hvset_{'a' if array else 's'}_{name}"
    args = (owner, idx, float(value)) if array else (owner, float(value))
    getattr(NEURON(), fn)(*args)


# HOC has no user-defined string-returning function syntax. Read a template
# strdef through a procedure that formats the member between the same sentinels
# as top-level strdef capture, and write it through a typed $s2 argument.
_HOC_TEMPLATE_STRING_TRAMPOLINES = set()


def _ensure_hoc_template_string_trampolines(name):
    if name in _HOC_TEMPLATE_STRING_TRAMPOLINES:
        return
    from . import NEURON, _STR_BEGIN, _STR_END

    n = NEURON()
    if not _HOC_TEMPLATE_STRING_TRAMPOLINES:
        n("strdef _mn_hs_capture")
    n(
        f'proc _mn_hsget_{name}() {{ '
        f'sprint(_mn_hs_capture, "%s%s%s", "{_STR_BEGIN}", '
        f'$o1.{name}, "{_STR_END}") print _mn_hs_capture }}'
    )
    n(f"proc _mn_hsset_{name}() {{ $o1.{name} = $s2 }}")
    _HOC_TEMPLATE_STRING_TRAMPOLINES.add(name)


def _read_hoc_template_string(owner, name):
    from . import NEURON, _capture_hoc_string

    _ensure_hoc_template_string_trampolines(name)
    n = NEURON()
    return _capture_hoc_string(lambda: getattr(n, f"_mn_hsget_{name}")(owner))


def _write_hoc_template_string(owner, name, value):
    if not isinstance(value, str):
        raise TypeError(f"argument must be str, not {type(value).__name__}")
    if "\x00" in value:
        raise ValueError("embedded null character")
    from . import NEURON

    _ensure_hoc_template_string_trampolines(name)
    getattr(NEURON(), f"_mn_hsset_{name}")(owner, value)


# StringFunctions alias support (obj.aliasname / obj.aliasname = v /
# obj._ref_aliasname). An alias created by `StringFunctions().alias(obj, name,
# ref)` lives in the object's RUNTIME alias table, not the class symbol table,
# so the native property accessors (nrn_property_get/set/push) can't address it
# — they dereference garbage and SIGSEGV. We route through HOC trampolines
# (`$o1.<name>`), which consult the alias table the way the interpreter does.
# One get/set trampoline pair is defined per alias name (the proc takes $o1, so
# it serves every object); a non-alias name makes HOC raise "... not a public
# member", which we convert to a clean AttributeError.
_OBJECT_ALIAS_TRAMPOLINES = set()


def _ensure_alias_trampolines(name):
    """Define the per-name get/set alias trampolines. Returns False for names
    that aren't usable HOC identifiers (dunders / private probes), so callers
    skip the alias path for them."""
    if not (name.isidentifier() and not name.startswith("_")):
        return False
    if name not in _OBJECT_ALIAS_TRAMPOLINES:
        from . import NEURON

        n = NEURON()
        n(f"func _mn_aliasget_{name}() {{ return $o1.{name} }}")
        n(f"proc _mn_aliasset_{name}() {{ $o1.{name} = $2 }}")
        _OBJECT_ALIAS_TRAMPOLINES.add(name)
    return True


class _SuppressCStderr:
    """Silence NEURON's hoc_execerror stderr during an alias probe that may
    fail on a non-alias name. The failed call is caught and turned into a clean
    AttributeError, but NEURON still prints "NEURON: '<name>' not a public
    member" to fd 2 alongside the caught exception — undesirable for a benign
    miss or typo. Redirect fd 2 to /dev/null for the wrapped call only; the
    success path produces no output, so this is invisible there. (An alias
    enumeration API — alias_list — needs a String template myneuron doesn't
    load, so probe-and-suppress is the available route.)"""

    __slots__ = ("_saved", "_devnull")

    def __enter__(self):
        import os

        self._saved = os.dup(2)
        self._devnull = os.open(os.devnull, os.O_WRONLY)
        os.dup2(self._devnull, 2)
        return self

    def __exit__(self, *exc):
        import os

        os.dup2(self._saved, 2)
        os.close(self._devnull)
        os.close(self._saved)
        return False


def _read_object_alias(obj, name):
    """Return the value of StringFunctions alias *name* on *obj*; raise
    AttributeError if *obj* has no such alias (the HOC '... not a public member'
    error, surfaced as RuntimeError by the dispatch)."""
    from . import NEURON

    if not _ensure_alias_trampolines(name):
        raise AttributeError(name)
    try:
        with _SuppressCStderr():
            return getattr(NEURON(), f"_mn_aliasget_{name}")(obj)
    except RuntimeError:
        raise AttributeError(name) from None


def _write_object_alias(obj, name, value):
    """Write *value* to StringFunctions alias *name* on *obj*. Returns True if
    written, False if *obj* has no such alias (or *value* isn't numeric)."""
    from . import NEURON

    if not _ensure_alias_trampolines(name):
        return False
    try:
        fv = float(value)
    except (TypeError, ValueError):
        return False
    try:
        with _SuppressCStderr():
            getattr(NEURON(), f"_mn_aliasset_{name}")(obj, fv)
        return True
    except RuntimeError:
        return False


class _AliasRef:
    """``obj._ref_<alias>`` for a StringFunctions alias. The alias' double*
    lives in the object's runtime alias table, which the native property-pointer
    API can't address (it SIGSEGVs), so the scalar-ref ``[0]`` read/write routes
    through the same HOC trampolines as the alias attribute. Holds the object
    alive (an alias ref is lightweight)."""

    __slots__ = ("_obj", "_name")

    def __init__(self, obj, name):
        self._obj = obj
        self._name = name

    def __getitem__(self, idx):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        return _read_object_alias(self._obj, self._name)

    def __setitem__(self, idx, value):
        if idx != 0:
            raise IndexError("Only index 0 is supported")
        if not _write_object_alias(self._obj, self._name, value):
            raise AttributeError(
                f"no alias '{self._name}' on {self._obj._class_name}"
            )

    def __repr__(self):
        return f"<AliasRef to {self._obj._class_name}.{self._name}>"


def _alias_ref(obj, name):
    """Return an _AliasRef if *name* is a StringFunctions alias on *obj*, else
    None (confirmed by probing the read trampoline)."""
    try:
        _read_object_alias(obj, name)
    except AttributeError:
        return None
    return _AliasRef(obj, name)


# ---------------------------------------------------------------------------
# Container subclasses — List, Vector, SectionList
# ---------------------------------------------------------------------------


class List(Object):
    """A HOC ``List`` — ordered, indexable container of HOC objects.

    Create with ``n.List()``. Supports ``len(lst)``, ``lst[i]``, and any
    HOC ``List`` method (``append``, ``remove``, ``count``, ``object``,
    etc.) via attribute dispatch.
    """

    __slots__ = ()
    _hoc_class_name = "List"

    def __bool__(self):
        return int(self.count()) > 0

    def __len__(self):
        return int(self.count())

    def __getitem__(self, i):
        if i < 0:
            i += len(self)
        if i < 0 or i >= len(self):
            raise IndexError("List index out of range")
        return self.object(i)


class _VectorXAccessor:
    """Proxy for Vector.x — delegates indexing to the parent Vector.

    Real NEURON's ``v.x[i]`` idiom: without this proxy, ``v.x`` falls through
    to HOC dispatch and returns a float (the HOC ``x`` method returns a double),
    breaking the ``v.x[i]`` syntax used pervasively in NEURON scripts.
    """

    __slots__ = ("_vec",)

    def __init__(self, vec):
        self._vec = vec

    def __getitem__(self, i):
        return self._vec[i]

    def __setitem__(self, i, val):
        self._vec[i] = val

    def __len__(self):
        return len(self._vec)

    def __repr__(self):
        return f"<Vector.x ({len(self._vec)} elements)>"


class Vector(Object):
    """A HOC ``Vector`` — dynamic array of doubles with 100+ HOC methods.

    Create with ``n.Vector()``, ``n.Vector(size)``, or ``n.Vector([1, 2, 3])``.
    Supports indexing (``v[0]``, ``v[-1]``), slicing, iteration, ``len()``,
    and any HOC ``Vector`` method (``max``, ``min``, ``sum``, ``mean``,
    ``sort``, ``record``, ``dot``, etc.) via attribute dispatch.
    """

    __slots__ = ()
    _hoc_class_name = "Vector"

    def __copy__(self):
        """Return an independent native Vector, matching ``Vector.c()``."""
        return self.c()

    def __deepcopy__(self, memo):
        """Return an independent native Vector and memoize the wrapper."""
        copied = self.c()
        memo[id(self)] = copied
        return copied

    @property
    def x(self):
        return _VectorXAccessor(self)

    def __getitem__(self, i):
        if isinstance(i, slice):
            return [self.get(idx) for idx in range(*i.indices(len(self)))]
        if i < 0:
            i += len(self)
        if i < 0 or i >= len(self):
            raise IndexError()
        return self.get(i)

    def __setitem__(self, i, val):
        if isinstance(i, slice):
            indices = range(*i.indices(len(self)))
            vals = list(val)
            if len(vals) != len(indices):
                raise IndexError(
                    f"attempt to assign sequence of size {len(vals)} "
                    f"to extended slice of size {len(indices)}"
                )
            for idx, v in zip(indices, vals):
                self.set(idx, v)
            return
        if i < 0:
            i += len(self)
        if i < 0 or i >= len(self):
            raise IndexError()
        self.set(i, val)

    def __iter__(self):
        for i in range(len(self)):
            yield self.get(i)

    @property
    def __array_interface__(self):
        """Numpy array-interface protocol.

        `numpy.array(vec)` copies (standard numpy semantics);
        `numpy.asarray(vec)` returns a **read-only** view. The
        read-only flag matches real NEURON's wrapper — users who want
        a mutable view should call `vec.as_numpy()` explicitly so
        accidental writes through `np.asarray` can't corrupt the
        Vector's storage.

        Empty vectors: returning `data=(0, True)` works on permissive
        numpy builds but the stricter ones (the GitHub Actions
        Ubuntu image among them) reject the NULL pointer and fall
        through to the sequence protocol, which then tries `float(vec)`
        and raises `TypeError: float() argument must be a string or
        a real number`. Borrow a real empty array's interface
        instead — numpy guarantees its own empty-array data pointer
        is safe to expose.

        The dict is rebuilt on every access so resizes between calls
        get fresh ('data', 'shape') values. Callers in tight loops
        should pin the result of ``numpy.asarray()`` once outside the
        loop, or use ``as_numpy()`` for the zero-copy mutable view.
        """
        import ctypes
        from .api import _nrn_vector_data
        size = len(self)
        if size == 0:
            import numpy as _np
            return _np.empty(0, dtype=_np.float64).__array_interface__
        ptr = _nrn_vector_data(self._obj)
        addr = ctypes.cast(ptr, ctypes.c_void_p).value or 0
        return {
            "version": 3,
            "shape": (size,),
            "typestr": "<f8",  # little-endian float64
            "data": (addr, True),  # (address, read-only=True)
        }

    def as_numpy(self):
        """Return a numpy array view of this Vector's data (zero-copy)."""
        import ctypes
        import numpy
        from .api import _nrn_vector_data

        size = len(self)
        if size == 0:
            return numpy.array([], dtype=float)
        ptr = _nrn_vector_data(self._obj)
        buf_from_mem = ctypes.pythonapi.PyMemoryView_FromMemory
        buf_from_mem.restype = ctypes.py_object
        # The CPython signature is `PyObject* PyMemoryView_FromMemory(
        # char* mem, Py_ssize_t size, int flags)`. `c_ssize_t` is
        # the ctypes alias for Py_ssize_t and is 64-bit on any 64-bit
        # build of Python — so a Vector with >2^31 doubles (16+ GB)
        # passes the byte count through without the silent truncation
        # we had with `c_int`. The third arg stays `c_int` (PyBUF_WRITE
        # = 0x200 fits in a regular int).
        buf_from_mem.argtypes = (ctypes.c_void_p, ctypes.c_ssize_t, ctypes.c_int)
        cbuffer = buf_from_mem(ptr, size * numpy.dtype(float).itemsize, 0x200)
        return numpy.ndarray((size,), float, cbuffer, order="C")

    def to_python(self, dest=None):
        """Return Vector contents as a Python list (or fill *dest*)."""
        from .api import _nrn_vector_data

        size = len(self)
        result = [] if size == 0 else _nrn_vector_data(self._obj)[:size]
        if dest is not None:
            dest[:] = result
            return dest
        return result

    def from_python(self, seq):
        """Fill this Vector from a Python sequence. Resizes to match."""
        seq = list(seq)
        self.resize(len(seq))
        for i, val in enumerate(seq):
            self.set(i, val)
        return self

    def __len__(self):
        return int(self.size())

    # --- Arithmetic (numeric protocol) ---
    # Mirrors real NEURON's `nrnpy_vec_math` callback (registered via
    # `nrnpy_vec_math_register`, called from `py_hocobj_math` in
    # src/nrnpython/nrnpy_hoc.cpp): forward ops clone via `.c()` then
    # call the HOC method; reversed `sub` and `div` use the
    # `mul(-1).add(x)` / `pow(-1).mul(x)` identities; unary `-`
    # uses `mul(-1)`. Operand must be a Vector or `numbers.Number`;
    # anything else returns `NotImplemented`.

    @staticmethod
    def _is_arith_operand(other):
        import numbers
        return isinstance(other, (Vector, numbers.Number))

    def __add__(self, other):
        if not Vector._is_arith_operand(other):
            return NotImplemented
        return self.c().add(other)

    def __radd__(self, other):
        if not Vector._is_arith_operand(other):
            return NotImplemented
        return self.c().add(other)

    def __iadd__(self, other):
        if not Vector._is_arith_operand(other):
            return NotImplemented
        self.add(other)
        return self

    def __sub__(self, other):
        if not Vector._is_arith_operand(other):
            return NotImplemented
        return self.c().sub(other)

    def __rsub__(self, other):
        if not Vector._is_arith_operand(other):
            return NotImplemented
        return self.c().mul(-1).add(other)

    def __isub__(self, other):
        if not Vector._is_arith_operand(other):
            return NotImplemented
        self.sub(other)
        return self

    def __mul__(self, other):
        if not Vector._is_arith_operand(other):
            return NotImplemented
        return self.c().mul(other)

    def __rmul__(self, other):
        if not Vector._is_arith_operand(other):
            return NotImplemented
        return self.c().mul(other)

    def __imul__(self, other):
        if not Vector._is_arith_operand(other):
            return NotImplemented
        self.mul(other)
        return self

    def __truediv__(self, other):
        if not Vector._is_arith_operand(other):
            return NotImplemented
        return self.c().div(other)

    def __rtruediv__(self, other):
        if not Vector._is_arith_operand(other):
            return NotImplemented
        return self.c().pow(-1).mul(other)

    def __itruediv__(self, other):
        if not Vector._is_arith_operand(other):
            return NotImplemented
        self.div(other)
        return self

    def __neg__(self):
        return self.c().mul(-1)

    def __pos__(self):
        return self.c()

    def __abs__(self):
        return self.c().abs()


class SectionList(Object):
    """HOC SectionList — collection of sections with morphology operations.

    Iteration uses the same C iterator API as allsec(), with a fresh
    iterator per __iter__ call so nested loops work.

    Morphology methods (append, remove, children, subtree, wholetree,
    allroots, unique) are inherited from the HOC object via Object
    method dispatch.

    Usage:
        sl = n.SectionList()                 # empty
        sl = n.SectionList([sec1, sec2])     # pre-populated
        sl.append(sec=soma)
        sl.wholetree(sec=soma)
        for sec in sl: ...
        soma in sl
        len(sl)
    """

    __slots__ = ()
    _hoc_class_name = "SectionList"

    def __iter__(self):
        from .api import (
            _nrn_sectionlist_data,
            _nrn_sectionlist_iterator_new,
            _nrn_sectionlist_iterator_next,
            _nrn_sectionlist_iterator_done,
            _nrn_sectionlist_iterator_free,
        )
        from .sections import Section

        items = _nrn_sectionlist_data(self._obj)
        it = _nrn_sectionlist_iterator_new(items)
        try:
            while not _nrn_sectionlist_iterator_done(it):
                sec_ptr = _nrn_sectionlist_iterator_next(it)
                # next() may discard dead entries and reach the sentinel.
                if not sec_ptr:
                    break
                yield Section._from_ptr(sec_ptr)
        finally:
            _nrn_sectionlist_iterator_free(it)

    def __len__(self):
        from .api import _nrn_sectionlist_data, _nrn_sectionlist_to_array

        items = _nrn_sectionlist_data(self._obj)
        return int(_nrn_sectionlist_to_array(items, None, 0))

    def __bool__(self):
        return len(self) != 0

    def __contains__(self, section):
        from .sections import Section

        if not isinstance(section, Section):
            return False
        for sec in self:
            if sec == section:
                return True
        return False

    def __getitem__(self, index):
        # SectionList is backed by a C linked list, so indexing is O(n).
        # For repeated random access, materialize once with list(sl).
        if isinstance(index, slice):
            return list(self)[index]
        if not isinstance(index, int):
            raise TypeError(
                f"SectionList indices must be int or slice, not {type(index).__name__}"
            )
        if index < 0:
            secs = list(self)
            try:
                return secs[index]
            except IndexError:
                raise IndexError("SectionList index out of range") from None
        for i, sec in enumerate(self):
            if i == index:
                return sec
        raise IndexError("SectionList index out of range")


# ---------------------------------------------------------------------------
# Template section helpers — _resolve_template_section, _TemplateSectionArray
# ---------------------------------------------------------------------------


def _resolve_template_section(owner, name, idx=None):
    """Push a HOC section onto the stack via braces and snapshot it.

    Used by type-307 (SECTION) dispatch for scalar `cell.soma`, indexed
    `cell.dend[i]`, and the top-level forms `n.soma` / `n.dend[i]` for
    HOC-`create`d sections. The braces auto-pop after the block, but the
    `_mn_capture_cas` proc runs inside the block so cas() is the section at
    that point. The captured raw Section* pointer remains valid because the
    template (or HOC top level) owns the section.

    `owner` is the template Object that owns the section, or None for a
    top-level `create`d section (the brace command then has no `owner.`
    prefix).
    """
    from . import _template_section_capture
    from .sections import Section
    from .api import _nrn_hoc_call

    # Save/restore the module-global capture slot and read the result into a
    # local: a Python callback that itself resolves a template section, if it
    # ran inside the brace block, would otherwise clobber our pending value.
    # Preserve this even for steering paths that do not currently run callbacks.
    saved = _template_section_capture.value
    _template_section_capture.value = 0
    suffix = f"[{int(idx)}]" if idx is not None else ""
    prefix = f"{owner.hname()}." if owner is not None else ""
    cmd = f"{prefix}{name}{suffix} {{ _mn_capture_cas() }}"
    try:
        ret = _nrn_hoc_call(cmd.encode("utf-8"))
        captured = _template_section_capture.value
    finally:
        _template_section_capture.value = saved
    if ret != 0 or not captured:
        scope = owner._class_name + "." if owner is not None else ""
        raise AttributeError(
            f"Could not access section '{scope}{name}{suffix}'"
        )
    return Section._from_ptr(captured)


# SECTIONREF steering trampolines (one proc per steering symbol). Cached so the
# proc is defined once. See _steer_sectionref.
_SECTIONREF_STEER_TRAMPOLINES = set()


def _steer_sectionref(sr_obj, which, index=None):
    """Resolve a SectionRef steering symbol to its Section via the HOC brace form.

    `sref.parent { ... }` is a parser steering construct: it makes the relative
    section (parent/root/trueparent/child, or the ref's own `sec`) the
    currently-accessed section for the following block. Calling the symbol
    through ``nrn_method_call`` does NOT steer — ``nrn_cas()`` afterwards returns
    the tree ROOT for sec/parent/root alike, which silently collapsed
    ``parentseg()`` (and everything built on it: children/subtree/wholetree/
    trueparentseg) onto the root. The brace + ``_mn_capture_cas`` form is the
    only correct path (same mechanism as ``_resolve_template_section``). Returns
    the Section, or None when the ref has no such relative (e.g. parent of a
    root, where the steering raises and nothing is captured).
    """
    from . import NEURON, _template_section_capture
    from .sections import Section
    from .utils import list_functions

    n = NEURON()
    key = "child" if index is not None else which
    proc = f"_mn_steer_{key}"
    if proc not in _SECTIONREF_STEER_TRAMPOLINES:
        steer = f"{which}[$2]" if index is not None else which
        n(f"proc {proc}() {{ $o1.{steer} {{ _mn_capture_cas() }} }}")
        _SECTIONREF_STEER_TRAMPOLINES.add(proc)
        n._top_level = list_functions()
    # Save/restore the capture slot + read into a local — reentrancy insurance,
    # same rationale as _resolve_template_section.
    saved = _template_section_capture.value
    _template_section_capture.value = 0
    try:
        if index is not None:
            getattr(n, proc)(sr_obj, index)
        else:
            getattr(n, proc)(sr_obj)
        captured = _template_section_capture.value
    finally:
        _template_section_capture.value = saved
    if not captured:
        return None
    return Section._from_ptr(captured)


# Per-member obfunc trampolines that return a template's objref member
# (type 324, OBJECTVAR). One trampoline serves every template with a public
# member of that name, because `$o1.<name>` resolves the member dynamically
# off whichever object is pushed. Caching avoids redefining the obfunc on
# every access. Mirrors __init__._OBJECTVAR_TRAMPOLINES (top-level objrefs).
_MEMBER_OBJ_TRAMPOLINES = set()


def _read_object_member(owner, name):
    """Return the Object bound to template objref member ``owner.name``.

    OBJECTVAR members can't be dispatched via ``nrn_method_call`` (the symbol
    is an object reference, not a method). Define
    ``obfunc _mn_memgrab_<name>() { return $o1.<name> }`` once, then call it
    with ``owner`` pushed as the argument; ``$o1.<name>`` resolves the public
    member dynamically. Used by ``Object.__getattr__``'s OBJECTVAR branch so
    e.g. an Import3d cell's ``cell.all`` returns its SectionList.
    """
    from . import NEURON
    from .utils import list_functions

    n = NEURON()
    fn = f"_mn_memgrab_{name}"
    idfn = f"_mn_memid_{name}"
    if fn not in _MEMBER_OBJ_TRAMPOLINES:
        n(f"obfunc {fn}() {{ return $o1.{name} }}")
        # Companion id func for the nil guard (see below).
        n(f"func {idfn}() {{ return object_id($o1.{name}) }}")
        _MEMBER_OBJ_TRAMPOLINES.add(fn)
        # Refresh the dispatch table so the new obfunc is callable.
        n._top_level = list_functions()
    # Nil guard: an unset objref member pops a nil Object* that segfaults
    # nrn_class_name (same non-NULL sentinel as top-level objrefs). object_id
    # is the crash-safe nil test; real NEURON returns None.
    if getattr(n, idfn)(owner) == 0.0:
        return None
    return getattr(n, fn)(owner)


def _read_object_member_index(owner, name, idx):
    """Return the Object at ``owner.name[idx]`` for an objref-array member, or None.

    Array analog of _read_object_member: `$o1.<name>[$2]` resolves the indexed
    member dynamically, with the same object_id nil guard.
    """
    from . import NEURON
    from .utils import list_functions

    n = NEURON()
    fn = f"_mn_memgrabidx_{name}"
    idfn = f"_mn_memidx_id_{name}"
    if fn not in _MEMBER_OBJ_TRAMPOLINES:
        n(f"obfunc {fn}() {{ return $o1.{name}[$2] }}")
        n(f"func {idfn}() {{ return object_id($o1.{name}[$2]) }}")
        _MEMBER_OBJ_TRAMPOLINES.add(fn)
        n._top_level = list_functions()
    if getattr(n, idfn)(owner, idx) == 0.0:
        return None
    return getattr(n, fn)(owner, idx)


class _ObjectMemberArray:
    """Subscriptable proxy for a template's ``objref name[N]`` member array.

    Mirrors real NEURON: ``cell.name[i]``, ``len()``, iteration; nil elements
    read as None. Holds the owner Object (which keeps the HOC object alive).
    """

    __slots__ = ("_owner", "_name", "_size")

    def __init__(self, owner, name, size):
        self._owner = owner
        self._name = name
        self._size = size

    def __getitem__(self, idx):
        if isinstance(idx, slice):
            return [self[i] for i in range(*idx.indices(self._size))]
        if isinstance(idx, bool) or not isinstance(idx, int):
            raise TypeError(
                f"objref array indices must be integers, not {type(idx).__name__}"
            )
        if idx < 0:
            idx += self._size
        if not 0 <= idx < self._size:
            raise IndexError(
                f"index {idx} out of range [0, {self._size}) for "
                f"{self._owner._class_name}.{self._name}"
            )
        return _read_object_member_index(self._owner, self._name, idx)

    def __len__(self):
        return self._size

    def __iter__(self):
        for i in range(self._size):
            yield self[i]


class _TemplateSectionArray:
    """Subscriptable proxy for a HOC `create name[N]` section array.

    Real NEURON returns a HocObject for `cell.dend` when dend is declared as
    `create dend[3]`; that object supports `cell.dend[i]`, `len()`, and
    iteration. We mirror that interface with a thin wrapper because
    nrn_method_call can't dispatch type-307 — see Object.__getattr__ for the
    callback-based access pattern. `owner` is the template Object, or None
    for a top-level `create dend[N]` array reached via `n.dend[i]`.
    """

    __slots__ = ("_owner", "_name", "_size")

    def __init__(self, owner, name, size):
        self._owner = owner
        self._name = name
        self._size = size

    def __getitem__(self, idx):
        if isinstance(idx, slice):
            return [self[i] for i in range(*idx.indices(self._size))]
        if isinstance(idx, bool) or not isinstance(idx, int):
            raise TypeError(
                f"section array indices must be integers, not {type(idx).__name__}"
            )
        if idx < 0:
            idx += self._size
        if not 0 <= idx < self._size:
            scope = (
                f"{self._owner._class_name}." if self._owner is not None else ""
            )
            raise IndexError(
                f"index {idx} out of range [0, {self._size}) for "
                f"{scope}{self._name}"
            )
        return _resolve_template_section(self._owner, self._name, idx=idx)

    def __len__(self):
        return self._size

    def __iter__(self):
        for i in range(self._size):
            yield self[i]

    def __repr__(self):
        # A top-level `create dend[2]` has no owning template, so _owner is None;
        # guard it the same way __getitem__ does rather than
        # dereferencing None._class_name.
        scope = f"{self._owner._class_name}." if self._owner is not None else ""
        return f"<template section array {scope}{self._name}[{self._size}]>"


# ---------------------------------------------------------------------------
# ArrayProperty
# ---------------------------------------------------------------------------


class ArrayProperty:
    """Subscriptable wrapper for an Object's array-valued property.

    Returned by Object.__getattr__ when the property's symbol is array-valued
    (detected via nrn_symbol_is_array + nrn_symbol_array_length).
    Example: VClamp has amp[3], dur[3]; scalar access (amp[0]) reads/writes
    the underlying double via nrn_property_array_get/set.
    """

    __slots__ = (
        "_owner",
        "_name",
        "_name_encoded",
        "_size",
        "_qualified_name",
        "_hoc_template_var",
    )

    # Retain the owner Object WRAPPER, not the bare obj* — same as
    # _ObjectMemberArray / _TemplateSectionArray. Holding only the pointer let
    # the owner be GC'd (its __del__ runs nrn_object_unref → the HOC object is
    # freed) while this proxy outlived it; the next array read then dereferenced
    # freed native memory and segfaulted (e.g. `a = vc.amp; del vc; a[0]`).
    def __init__(
        self, owner, name, size, qualified_name=None, hoc_template_var=False
    ):
        self._owner = owner
        self._name = name
        self._name_encoded = name.encode("utf-8") if isinstance(name, str) else name
        self._size = size
        self._qualified_name = qualified_name or name
        self._hoc_template_var = hoc_template_var

    def _resolve(self, idx):
        # numbers.Integral accepts numpy.int64/int32 (not int subclasses) the
        # way real NEURON does; bool stays rejected.
        import numbers

        if isinstance(idx, bool) or not isinstance(idx, numbers.Integral):
            raise TypeError(
                f"indices must be integers, not {type(idx).__name__}"
            )
        idx = int(idx)
        if idx < 0:
            idx += self._size
        if not 0 <= idx < self._size:
            raise IndexError(
                f"index {idx} out of range [0, {self._size}) "
                f"for array property {self._qualified_name}"
            )
        return idx

    def __getitem__(self, idx):
        from .api import _property_array_get_checked

        idx = self._resolve(idx)
        if self._hoc_template_var:
            return float(_read_hoc_template_var(self._owner, self._name, idx))
        return float(
            _property_array_get_checked(self._owner._obj, self._name_encoded, idx)
        )

    def __setitem__(self, idx, value):
        from .api import _property_array_set_checked

        idx = self._resolve(idx)
        if self._hoc_template_var:
            _write_hoc_template_var(self._owner, self._name, value, idx)
            return
        _property_array_set_checked(
            self._owner._obj, self._name_encoded, idx, float(value)
        )

    def __len__(self):
        return self._size

    def __iter__(self):
        for i in range(self._size):
            yield self[i]

    def __repr__(self):
        try:
            vals = list(self)
        except Exception:
            vals = "?"
        return f"<ArrayProperty {self._qualified_name} {vals}>"
