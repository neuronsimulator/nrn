"""Section, Segment wrappers — topology, range variables, tree traversal, psection."""
# Module-level imports only from modules that never import this one (api,
# nrnref, utils): a function-level import cost ~50% of seg.v time
# (benchmarks/README.md#historical-implementation-measurements). Imports from
# mechanism, object and the package are reached through _PACKAGE at call time
# (a cycle forbids importing them here).
import ctypes
import math
import numbers
import operator
import weakref

from .api import (
    CALLS_PYTHON,
    EPOCH,
    NRN_CLASS_MODULE,
    HOC_LOCK,
    SECTION_BASE,
    SEGMENT_BASE,
    _bump_model_epoch,
    _model_epoch,
    in_hoc_call,
    _refused_double,
    _to_double,
    TYPES,
    _ERR_BUF,
    _ERR_BUF_SIZE,
    _check_nrn_error,
    _TREE_SET_UP,
    _ensure_topology,
    _fast_imem_on,
    _nrn_double_pop,
    _nrn_double_push,
    _nrn_function_call,
    _nrn_str_push,
    _diam_changed as _DIAM_CHANGED,
    _nrn_double_ptr_pop,
    _nrn_mechanism_insert,
    _nrn_nseg_get,
    _nrn_nseg_set,
    _nrn_rangevar_get as _RANGEVAR_GET,
    _nrn_rangevar_push,
    _nrn_rangevar_set as _RANGEVAR_SET,
    _nrn_section_child,
    _nrn_section_connect,
    _nrn_section_diam_get as _SEG_DIAM_GET,
    _nrn_section_diam_set as _SEG_DIAM_SET,
    _nrn_section_is_active,
    _nrn_secname,
    _nrn_section_length_get,
    _nrn_section_length_set,
    _nrn_section_ref,
    _nrn_section_parent,
    _nrn_section_pop,
    _nrn_section_push,
    _nrn_section_Ra_get,
    _nrn_section_Ra_set,
    _nrn_section_rallbranch_get,
    _nrn_section_rallbranch_set,
    _nrn_section_sibling,
    _nrn_segment_mechanisms,
    _nrn_segment_node_index,
    _nrn_symbol,
    _nrn_symbol_array_length,
    _nrn_symbol_is_array,
    _nrn_symbol_name,
    _nrn_symbol_subtype,
    _nrn_symbol_type,
    Symbol,
    _diam_changed,
    _structure_change_count,
    _kschan_symbol_changes,
    get_topology_version,
    _PACKAGE,
)
from .nrnref import NrnRangeArrayRef, NrnRangeVarRef, NrnRef, _sequence_index
from .utils import (
    _DELETED_SECTION,
    _drop_return_slot,
    _name_error,
    _pop_args,
    _pop_pushed_section,
    _try_wrap_density_nmodlrandom,
)
from . import utils as _utils

# Names with registered get/set procs; see _ArrayRangeVar.
_ARRAY_RANGEVAR_PROCS = set()


# Symbol-info cache for Segment.__getattr__/__setattr__. Maps a name (str)
# to (sym_ptr, sym_type, is_array, array_length).
#
# WHY: the encode + nrn_symbol + nrn_symbol_type + nrn_symbol_is_array +
# nrn_symbol_array_length chain costs ~3.5us of the ~4.7us seg.v total. Real
# NEURON uses pmech_types/rangevars_ Python dicts for O(1) lookup
# (9.0's nrnpy_nrn.cpp).
# INVALIDATES: never. Global HOC symbols are stable for the process lifetime
# (once `gnabar_hh` exists its Symbol* never moves).
_SYMBOL_INFO_CACHE = {}
# Names that resolved to no symbol, with the model epoch of the lookup. A
# miss holds only until the epoch moves: a symbol can appear later
# (KSChan.add_hhstate, nrn_load_dll, HOC code), and every such call moves it.
_SYMBOL_MISS_EPOCH = {}
_V_SYMBOL = _nrn_symbol(b"v")
_SYMBOLS_SEEN = [None]


def _symbol_is_current(name, sym):
    """Whether the symbol kept for *name* is still the one the core finds.

    KSChan.iv_type() renames gmax_<mech> to pmax_<mech> in place and back; the
    Symbol stays the same variable, and NEURON's Python module keeps reading it
    by the old name (test_kschan.py test_4), so the other spelling counts too.
    """
    if _nrn_symbol(name.encode("utf-8")) == sym:
        return True
    prefix, _, rest = name.partition("_")
    other = {"gmax": "pmax", "pmax": "gmax"}.get(prefix)
    return other is not None and _nrn_symbol(f"{other}_{rest}".encode("utf-8")) == sym


def forget_moved_symbols():
    """Forget every name kept with a Symbol* that KSChan renamed or freed.

    KSChan.name() renames a mechanism's symbols in place, and ion() and state
    changes free them, so a name kept here would read another mechanism's
    variable or freed memory. The core counts those changes. When the count
    moved, each kept name is looked up again (nrn_symbol) and dropped if its
    symbol differs. As in NEURON's Python module, a name whose symbol KSChan
    only switched between gmax and pmax keeps working. On a core without the
    count nothing is forgotten (TODO(gap-94)). The core clears its fast-path
    tables on the same count; this module refills them.
    """
    if _kschan_symbol_changes is None:
        return
    count = _kschan_symbol_changes()
    since = _SYMBOLS_SEEN[0]
    if count == since:
        return
    _SYMBOLS_SEEN[0] = count
    if since is None:  # every slow path looks first: nothing was kept before
        return
    moved = [
        n
        for n, info in _SYMBOL_INFO_CACHE.items()
        if not _symbol_is_current(n, info[0])
    ]
    for name in moved:
        del _SYMBOL_INFO_CACHE[name]
        _POINTER_RANGEVARS.pop(name, None)
    _SYMBOL_MISS_EPOCH.clear()
    mechanism = getattr(_PACKAGE, "mechanism", None)
    if mechanism is not None:
        for cache in mechanism._MECH_ATTR_CACHE.values():
            for name in [
                n for n, e in cache.items() if _nrn_symbol(e[0].encode("utf-8")) != e[1]
            ]:
                del cache[name]
        mechanism._DECLARATION_ORDER.clear()
    if _FAST_RANGEVARS is not None:
        _FAST_RANGEVARS.clear()
        _FAST_MECHANISMS.clear()


# The core's fast path (prototype, nrnpy_fast.cpp): tables this module fills
# after its own lookup resolved a name once. None on a core without it.
# name -> Symbol address of a scalar range variable that is not diam or a
# POINTER: the core reads and writes it at an interior x, and calls _getattr /
# _setattr for anything else. Cleared when KSChan renames or frees a symbol.
_FAST_RANGEVARS = getattr(SEGMENT_BASE, "_fast_rangevars", None)
# mechanism name -> {unsuffixed name -> Symbol address}, filled by mechanism.py.
# The core clears both when KSChan renames or frees a symbol.
_FAST_MECHANISMS = getattr(SEGMENT_BASE, "_fast_mechanisms", None)
if _FAST_RANGEVARS is not None:
    SEGMENT_BASE._fast_epoch = EPOCH
# A Segment slot write that skips Segment._setattr. object.__setattr__ refuses
# to bypass the core's setattro, whose own generic write this is.
_segment_set_slot = (
    object.__setattr__ if _FAST_RANGEVARS is None else SEGMENT_BASE.__setattr__
)
_CONNECT_ARG_UNSET = object()


def _cell_entry(sec):
    """The weakref to the cell of Section(cell=...), on a released core a tuple
    of the name's prefix and it, or None.

    An unset slot (wrappers from _from_ptr) means no cell. Uses
    object.__getattribute__ so the unset read does not recurse through
    __getattr__.
    """
    try:
        return object.__getattribute__(sec, "_cell")
    except AttributeError:
        return None


def _python_cell(sec):
    """The cell object passed to Section(cell=...), or None."""
    entry = _cell_entry(sec)
    if entry is None:
        return None
    return entry() if CALLS_PYTHON else entry[1]()


def _is_finite(value):
    """math.isfinite, false for a non-number: NEURON raises ValueError there."""
    try:
        return math.isfinite(value)
    except TypeError:
        return False


def _is_rangevar_name(name):
    """True if `name` (or its _ref_ form) is a range variable, which a Section
    attribute assignment sets on every segment."""
    info = _get_symbol_info(name[5:] if name.startswith("_ref_") else name)
    return info is not None and info[1] == TYPES.RANGEVAR


def _current_array_length(info):
    # An array's length is not stable: nlayer_extracellular() resizes the
    # extracellular arrays. Re-read it on every access; scalars stay cached.

    sym, sym_type, _, _ = info
    return (sym, sym_type, True, int(_nrn_symbol_array_length(sym)))


def _get_symbol_info(name):
    """Return (sym_ptr, sym_type, is_array, array_length) for *name*, or None
    if no such symbol exists. Cached after first lookup."""
    forget_moved_symbols()
    cached = _SYMBOL_INFO_CACHE.get(name)
    if cached is not None:
        return _current_array_length(cached) if cached[2] else cached
    epoch = EPOCH[0] or _model_epoch()
    if _SYMBOL_MISS_EPOCH.get(name) == epoch:
        return None

    sym = _nrn_symbol(name.encode("utf-8"))
    if not sym:
        _SYMBOL_MISS_EPOCH[name] = epoch
        return None
    _SYMBOL_MISS_EPOCH.pop(name, None)
    sym_type = int(_nrn_symbol_type(sym))
    is_array = bool(_nrn_symbol_is_array(sym))
    array_length = int(_nrn_symbol_array_length(sym)) if is_array else 0
    info = (sym, sym_type, is_array, array_length)
    _SYMBOL_INFO_CACHE[name] = info
    return info


def _rangevar_info(name):
    """_get_symbol_info(name) if *name* is a range variable, else None.

    Check this before making a range-variable reference: the core's range
    variable accessors, given any other symbol (an NMODL function, a global
    such as celsius, a builtin such as sin), crash the process.
    """
    info = _get_symbol_info(name)
    if info is None or info[1] != TYPES.RANGEVAR:
        return None
    return info


def _scalar_rangevar_double(name, value):
    """``value`` as a double if ``name`` is a scalar range variable, else as is.

    Conversion can run user code (``__float__``) that deletes the section or
    uninserts the mechanism. Callers convert first, before any liveness or
    mechanism check, and skip this call for a plain float or int.
    """
    info = _rangevar_info(name)
    if info is None or info[2]:
        return value
    return _to_double(value, ValueError)


def _install_array_rangevar_procs(name):
    if name in _ARRAY_RANGEVAR_PROCS:
        return

    n = _PACKAGE._n
    # $1 = array index, $2 = segment position x, $3 = value (set only)
    n(f"func _mn_rv_get_{name}() {{ return {name}[$1]($2) }}")
    n(f"proc _mn_rv_set_{name}() {{ {name}[$1]($2) = $3 }}")
    _ARRAY_RANGEVAR_PROCS.add(name)


# A mechanism POINTER variable (membfunc.h NRNPOINTER). nrn_rangevar_get/set
# dereference it unchecked, and the core throws through ctypes (process abort)
# when it was never set. Small HOC helpers called through the nothrow function
# API report NEURON's "wasn't made to point to anything" instead.
_NRNPOINTER = 4
_POINTER_RANGEVARS = {}
_POINTER_RANGEVAR_PROCS = set()


def _is_pointer_rangevar(name, sym):
    flag = _POINTER_RANGEVARS.get(name)
    if flag is None:
        flag = _POINTER_RANGEVARS[name] = int(_nrn_symbol_subtype(sym)) == _NRNPOINTER
    return flag


def _pointer_rangevar_procs(name):
    n = _PACKAGE._n
    if name not in _POINTER_RANGEVAR_PROCS:
        # $1 = segment position x, $2 = value (set only)
        n(f"func _mn_ptr_get_{name}() {{ return {name}($1) }}")
        n(f"proc _mn_ptr_set_{name}() {{ {name}($1) = $2 }}")
        _POINTER_RANGEVAR_PROCS.add(name)
    return n


class OpaquePointer:
    """A POINTER holding a non-double handle (e.g. a BBCOREPOINTER stream)."""

    __slots__ = ()


OpaquePointer.__module__ = "nrn"


def _call_pointer_proc(section, x, name, proc, *args):
    n = _pointer_rangevar_procs(name)
    try:
        # NEURON raises these without printing a HOC error trace; suppress the
        # helper's trace to match.
        with _PACKAGE.object._SuppressCStderr():
            return getattr(n, f"{proc}_{name}")(x, *args, sec=section)
    except RuntimeError as exc:
        # The core's messages are the only signal for these two states; no
        # public nothrow range-variable accessor reports them.
        message = str(exc)
        if "cannot be converted to data_handle<double>" in message:
            return OpaquePointer()
        if "point to anything" in message:
            raise AttributeError(
                f"{name} was not made to point to anything at {section.name()}({x})"
            ) from None
        raise


def _pointer_rangevar_get(section, x, name):
    value = _call_pointer_proc(section, x, name, "_mn_ptr_get")
    return value if isinstance(value, OpaquePointer) else float(value)


def _pointer_rangevar_set(section, x, name, value):
    if isinstance(value, bool) or not isinstance(value, numbers.Real):
        raise ValueError(f"bad value for {name}: must be a double")
    if isinstance(
        _call_pointer_proc(section, x, name, "_mn_ptr_set", float(value)),
        OpaquePointer,
    ):
        raise ValueError("can't assign value to opaque pointer")


# ── _ArrayRangeVar ────────────────────────────────────────────────────────────


def _diam_node_x(sec, x):
    # The C diam accessors use node_exact (nrnoc/cabcode.cpp:1800), but Python
    # diameter access resolves the 0/1 ends to the adjacent interior segment
    # (the ends are a shared parent or terminal node, not diam storage). Reads
    # and writes must remap identically, or a written endpoint reads back the
    # unrelated end-node value.
    if x == 0.0:
        return 0.5 / sec.nseg
    if x == 1.0:
        return 1.0 - 0.5 / sec.nseg
    return x


def _diam_hoc_assign(sec, cmd, x, value):
    # HOC assignment reaches nrn_diam_change via range_interpolate_single
    # (nrnoc/cabcode.cpp:1097), keeping the core's segment-to-pt3d mapping and
    # length adjustment. Issued for every section: on a no-3d section it
    # rewrites the same node, and gating on sec.n3d() cost more than it saved
    # (~10 us full HOC dispatch vs ~6 us for this call). The raw node store
    # happens first because pt3dconst(1) suppresses the HOC assignment while
    # Python must still write the node and dirty the geometry.

    try:
        _PACKAGE._n(cmd, sec=sec, raise_on_error=True, _no_epoch_bump=True)
    except RuntimeError as exc:
        raise ValueError(
            f"diam assignment failed for {sec.name()}({x}) = {value!r}"
        ) from exc


class _ArrayRangeVar:
    """Subscriptable wrapper for an array-valued range variable on a segment.

    Used for the extracellular mechanism (xraxial/xg/xc). The C API exposes
    nrn_rangevar_get/set/push only for scalar symbols and HOC cannot
    dereference a range variable through a string arg ($s1), so access goes
    through one get/set HOC proc pair per name, registered on first use.
    """

    __slots__ = ("_section", "_x", "_name", "_size")

    def __reduce_ex__(self, protocol):
        raise TypeError("cannot pickle 'nrn.RangeVar' object")

    def __init__(self, section, x, name, size):
        self._section = section
        self._x = x
        self._name = name
        self._size = size

    def _resolve(self, idx):
        i = _sequence_index(idx, self._size, "indices must be integers, not {}")
        if i is None:
            raise IndexError(
                f"index {idx} out of range [0, {self._size}) "
                f"for array range variable {self._name}"
            )
        return i

    def _call(self, proc, *args):
        """Run the HOC get or set helper for this variable at this segment.

        The helper reports an absent mechanism as a HOC error (and, under
        MPI, a HOC error can abort), so the mechanism is checked first and
        NEURON's AttributeError raised. vext is the exception: NEURON reads it
        at the exact node (at a child's 0 end that is the parent's node) and
        gives 0 at a node without extracellular layers when a child section
        of that node's section has them (nrn_vext_pd), which no check on this
        section can tell. For vext the helper itself asks, quietly, and its
        HOC error becomes the AttributeError.
        """

        self._section._check_alive()
        # NEURON's section-loop error, not the helper's HOC error.
        _ensure_rangevar_topology(self._section._sec, self._x, self._name)
        _install_array_rangevar_procs(self._name)
        helper = getattr(_PACKAGE._n, f"{proc}_{self._name}")
        if self._name != "vext":
            Segment(self._section, self._x)._check_rangevar_mechanism(self._name)
            return helper(*args, sec=self._section)

        try:
            with _PACKAGE.object._SuppressCStderr():
                return helper(*args, sec=self._section)
        except RuntimeError:
            segment = Segment(self._section, self._x)
            raise AttributeError(
                f"vext, the mechanism does not exist at {segment!r}"
            ) from None

    def __getitem__(self, idx):
        idx = self._resolve(idx)
        return float(self._call("_mn_rv_get", idx, self._x))

    def __setitem__(self, idx, value):
        value = _to_double(value, ValueError)
        if not _nrn_section_is_active(self._section._sec):
            raise ReferenceError(f"nrn.RangeVar {_DELETED_SECTION}")
        idx = self._resolve(idx)
        self._call("_mn_rv_set", idx, self._x, float(value))

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
        # Section.__repr__ prints a deleted section as "<deleted section>"
        # where name() raises, and uses Section.name, not a subclass's.
        sec = Section.__repr__(self._section)
        return f"<ArrayRangeVar {self._name}@{sec}({self._x}) {vals}>"


class _SectionArrayRangeVar(_ArrayRangeVar):
    """sec.xg: reads are the middle segment's, writes reach every segment.

    As NEURON's: sec.xg[i] = v is a section-level assignment (the way
    sec.gnabar_hh = v is), where seg.xg[i] = v is one segment's.
    """

    __slots__ = ()

    def __setitem__(self, idx, value):
        value = _to_double(value, ValueError)
        sec = self._section
        if not _nrn_section_is_active(sec._sec):
            raise ReferenceError(f"nrn.RangeVar {_DELETED_SECTION}")
        idx = self._resolve(idx)
        name = self._name
        size = self._size
        # The middle segment first, so an absent mechanism is reported there
        # as NEURON does; then every node, both ends too (vext lives there).
        for x in [self._x] + [seg._x for seg in sec.allseg()]:
            _ArrayRangeVar(sec, x, name, size)._call("_mn_rv_set", idx, x, float(value))


# ── Segment ───────────────────────────────────────────────────────────────────


_UNSET = object()
_object_setattr = object.__setattr__
_STALE = object()  # _current_inserted_mechs: built at an older epoch

# i_membrane_ is no mechanism's: it exists while cvode.use_fast_imem(1) is on.
_FAST_IMEM = object()

# Range variable name -> the density mechanism that owns it ("gnabar_hh" ->
# "hh", "ena" -> "na_ion", "i_membrane" -> "extracellular"), None for one
# that needs no check, or _FAST_IMEM for i_membrane_. As in NEURON, the owner
# is the mechanism whose own variable list declares the name. Reading a
# variable whose owner is absent throws through ctypes, so every read and
# write checks the owner first.
# Filled in per name, starting from NEURON's built-in range variables
# (remake_pmech_types in 9.0's nrnpy_nrn.cpp adds them by name): v is every node's,
# and diam, cm and i_cap are morphology's and capacitance's, which every
# section has. vext is extracellular's, but NEURON reads it at the exact node,
# not the section, so _ArrayRangeVar asks NEURON itself.
# INVALIDATES: never. Only names that are range-variable symbols are looked
# up, and a mechanism loaded later cannot declare a symbol that already exists.
_RANGEVAR_OWNER = {
    "v": None,
    "diam": None,
    "cm": None,
    "i_cap": None,
    "i_membrane_": _FAST_IMEM,
    "vext": None,
}
# Variables of these are present in every section.
_EVERY_SECTION_MECHANISMS = frozenset({"morphology", "capacitance"})


def _is_density_mechanism(name):
    if name in _EVERY_SECTION_MECHANISMS:
        return False
    sym = _nrn_symbol(name.encode("utf-8"))
    return bool(sym) and int(_nrn_symbol_type(sym)) == TYPES.MECHANISM


def _find_owner(name):
    """The mechanism whose variable list declares range variable *name*.

    Reads the lists of only the mechanisms the name can belong to, each once
    (mechanism._declaration_order keeps them).
    """

    # NMODL names a variable <name>_<suffix>, and a suffix can itself contain
    # "_" (na_slow, or a KSChan named k_dr): every density mechanism whose
    # name ends this one, longest first.
    candidates = [
        name[i + 1 :]
        for i in range(1, len(name) - 1)
        if name[i] == "_" and _is_density_mechanism(name[i + 1 :])
    ]
    for refresh in (False, True):  # a KSChan gains a variable with each gate
        for mech in candidates:
            if name in (_PACKAGE.mechanism._declaration_order(mech, refresh) or ()):
                return mech
    # No suffix: an ion's variable (ena) or extracellular's (i_membrane).
    names = _PACKAGE.mechanism._all_density_mechanism_names()  # in type order
    for mech in sorted(names, key=lambda name: not name.endswith("_ion")):
        if mech not in candidates and name in (
            _PACKAGE.mechanism._declaration_order(mech) or ()
        ):
            return mech
    # Declared by no list read: check the likeliest mechanism, which can at
    # worst raise AttributeError where reading would have worked.
    return candidates[0] if candidates else None


def _rangevar_owner(name):
    """The owner of range variable *name*, as _RANGEVAR_OWNER holds it."""
    owner = _RANGEVAR_OWNER.get(name, _UNSET)
    if owner is _UNSET:
        owner = _RANGEVAR_OWNER[name] = _find_owner(name)
    return owner


def _missing_owner(sec, name):
    """The owner (see _RANGEVAR_OWNER) of range variable *name* if *sec* lacks
    it, else None.

    Asked every time: reading an absent mechanism raises NEURON's error
    through ctypes, and the model epoch does not move on every path that
    removes one (HOC run from nrniv, a callback inside the HOC call that
    uninserted; see gap-68), so the answer is not kept.
    """
    owner = _RANGEVAR_OWNER.get(name, _UNSET)
    if owner is _UNSET:
        owner = _rangevar_owner(name)
    if owner is None:
        return None
    if owner is _FAST_IMEM:
        return None if _fast_imem_on(sec._sec) else owner
    return None if _ismembrane(sec._sec, owner) else owner


def _ensure_rangevar_topology(sec_ptr, x, name):
    """api._ensure_topology before a raw access to range variable *name*.

    Only where NEURON's access runs it: nrn_rangepointer (cabcode.cpp:
    1274-1303) finds the node with node_exact for v, i_membrane_ and
    extracellular's variables. The others index the section's own nodes
    (node_index), which never sets the tree up, and NEURON reads them on a
    looped tree.
    """
    if 0.0 < x < 1.0:
        return
    if name in ("v", "vext") or _rangevar_owner(name) in (_FAST_IMEM, "extracellular"):
        _ensure_topology(sec_ptr, x)


def _current_inserted_mechs(sec):
    """Section *sec*'s cached _get_inserted_mechs list if it is current,
    _STALE if it was built before the model epoch last moved, None if never
    built.

    Makes no HOC call. Not a Section method: NEURON's test_deleted_sec expects
    every Section method to raise on a deleted section, and this one doesn't
    look at the section. Read with object.__getattribute__: the slots can be
    unset on wrappers from `_from_ptr`.
    """
    try:
        cached = object.__getattribute__(sec, "_inserted_mechs")
        built = object.__getattribute__(sec, "_inserted_mechs_epoch")
    except AttributeError:
        return None
    if cached is None:
        return None

    return cached if built == EPOCH[0] else _STALE


def _has_mechanism(sec, name):
    """Whether *sec* has density mechanism *name*: from its current cached
    list when there is one, else one has_membrane call."""
    inserted = _current_inserted_mechs(sec)
    if isinstance(inserted, list):
        return name in inserted
    return sec.has_membrane(name)


def _end_node_mechanisms(seg):
    """For a segment at x = 0 or 1: the density mechanisms NEURON finds at
    that node when it is a zero-area node, or None where the section's own
    mechanisms are used (see Segment._zero_area_owner)."""
    _ensure_topology(seg._sec._sec, seg._x)  # the node is found with node_exact
    owner = seg._zero_area_owner()
    if owner is None:
        return None
    return seg._zero_area_mechanisms(owner)


_SECTION_QUERY_SYMBOLS = {}


def _section_query(name, sec_ptr, *args):
    """A HOC builtin that only reads the accessed section
    (section_orientation, parent_connection, n3d, arc3d), as a float.
    ``args`` are pushed as doubles.

    Called directly, not through FuncWrapper: that moves the model epoch on
    every call, and rebuilding the mechanism caches it invalidates costs one
    ismembrane call per mechanism (12 -> 440 us for list(seg) at an end).
    """
    sym = _SECTION_QUERY_SYMBOLS.get(name)
    if sym is None:
        sym = _SECTION_QUERY_SYMBOLS[name] = _nrn_symbol(name)
    return _guarded_query(sym, sec_ptr, [("double", a) for a in args])


def _push_query_args(pushed):
    """Push the arguments *pushed* describes, counting before each push. A
    function of its own: an interrupt at the loop's back jump, inside
    _guarded_query's own frame, skipped its handler on Python 3.12."""
    for kind, payload in pushed[1]:
        pushed[0] += 1
        if kind == "double":
            _nrn_double_push(payload)
        else:
            _nrn_str_push(payload)


def _guarded_query(sym, sec_ptr, descriptors):
    """Push the section and *descriptors*, call *sym*, pop its double result,
    by the interrupt rule of NEURON.__getattr__'s nrn_func (gap-84): the
    counts are set before each push, a failed or interrupted call rolls the
    arguments back, and the section pop is retried from an inner finally."""
    pushed = [0, descriptors]
    pushed_sec = None
    err = _ERR_BUF()
    err[0] = b"\x01"  # the call clears it
    with HOC_LOCK:
        try:
            pushed_sec = [sec_ptr]
            _nrn_section_push(sec_ptr)
            _push_query_args(pushed)
            ret = _nrn_function_call(sym, len(descriptors), err, _ERR_BUF_SIZE)
            if ret:  # a failed call leaves its arguments on the stack
                _pop_args(pushed)
                failed, err = err, None
                _check_nrn_error(ret, failed)
            err = None
            return _nrn_double_pop()  # an interrupt here is still inside the try
        except BaseException:
            _drop_return_slot(err, _nrn_double_pop, pushed)
            raise
        finally:
            if pushed_sec:
                try:
                    _pop_pushed_section(pushed_sec)
                finally:
                    if pushed_sec:  # interrupted before the pop
                        _pop_pushed_section(pushed_sec)


_NAME_ARGS = {}  # mechanism name -> (c_char_p, its pointer), alive for pushes


def _ismembrane(sec_ptr, name):
    """Whether *sec_ptr* has the mechanism *name* (an ASCII str), as HOC's
    ismembrane() says, called as _section_query does: this is the check behind
    every seg.<mech var> read and write."""
    if not _nrn_section_is_active(sec_ptr):  # as n.ismembrane(sec=) checked
        raise ReferenceError(_DELETED_SECTION)
    arg = _NAME_ARGS.get(name)
    if arg is None:
        c_name = ctypes.c_char_p(name.encode("utf-8"))
        arg = _NAME_ARGS[name] = (c_name, ctypes.pointer(c_name))
    sym = _SECTION_QUERY_SYMBOLS.get(b"ismembrane")
    if sym is None:
        sym = _SECTION_QUERY_SYMBOLS[b"ismembrane"] = _nrn_symbol(b"ismembrane")
    return _guarded_query(sym, sec_ptr, [("str", arg[1])]) != 0


def _connect_forms_loop(child_ptr, parent_ptr, parent_x):
    """Whether NEURON refuses connecting the child at parent(parent_x).

    Mirrors nrn_parent_info (cabcode.cpp:1601-1608), which nrn_section_connect
    runs: walking up from the new parent while each connection is at its
    parent's 0 end (its orientation), reaching the child is a loop. The core
    raises that as a C++ throw, which aborts the process across ctypes.
    """
    if parent_ptr == child_ptr:
        return True
    if not _nrn_section_child(child_ptr):  # no one's ancestor
        return False
    sec_ptr, x = parent_ptr, parent_x
    while sec_ptr:
        if sec_ptr == child_ptr:
            return True
        if x != _section_query(b"section_orientation", sec_ptr):
            return False
        x = _section_query(b"parent_connection", sec_ptr)
        sec_ptr = _nrn_section_parent(sec_ptr)
    return False


# Located (non-ARTIFICIAL_CELL) point-process types, name -> MechanismType
# select index, and the type count it was built for. A MechanismType keeps the
# type list it was created with, so each call makes a fresh one (about 20 us);
# a new count (nrn_load_dll, make_pointprocess) rebuilds the map.
_PP_TYPES = [None, -1]


def _point_process_types():
    n = _PACKAGE._n
    mt = n.MechanismType(1)
    count = int(mt.count())
    if count == _PP_TYPES[1]:
        return mt, _PP_TYPES[0]
    selected = n.ref("")
    located = {}
    for i in range(count):
        if mt.is_artificial(i):
            continue
        mt.select(i)
        mt.selected(selected)
        located[selected[0]] = i
    _PP_TYPES[:] = [located, count]
    return mt, located


def _scaled_frustum_volume(length, d0, d1):
    """A frustum's volume without its pi/12 factor (scaled_frustum_volume in
    9.0's nrnpy_nrn.cpp)."""
    return length * (d0 * d0 + d0 * d1 + d1 * d1)


def _interpolate(x0, x1, y0, y1, xnew):
    """y at *xnew* on the line through (x0, y0) and (x1, y1), or y0 when
    x0 == x1 (interpolate in 9.0's nrnpy_nrn.cpp)."""
    if x1 == x0:
        return y0
    return y0 + (y1 - y0) * (xnew - x0) / (x1 - x0)


def _frustum_volume(sec, n3d, left_arc, right_arc):
    """Volume of *sec*'s 3D frustums between two arc lengths, in um^3.

    The 3D branch of seg_volume in 9.0's nrnpy_nrn.cpp, in the same
    arithmetic order. The diameter is interpolated at both arc lengths;
    diam3d already returns the |d| that code uses. A bisection finds the
    first point, so only the points inside the range are read.
    """
    sec_ptr = sec._sec
    # arg_bisect_arc3d: the last point before left_arc, or point 0.
    left, right = 0, n3d
    while right - left > 1:
        mid = (left + right) // 2
        if _section_query(b"arc3d", sec_ptr, mid) < left_arc:
            left = mid
        else:
            right = mid
    if left + 1 == n3d:
        # Every point is before the range: coincident points, L clamped to
        # 1e-9. NEURON reads one point past the end for a diameter that
        # nothing then uses, and returns 0.
        return 0.0
    left_diam = _interpolate(
        _section_query(b"arc3d", sec_ptr, left),
        _section_query(b"arc3d", sec_ptr, left + 1),
        _section_query(b"diam3d", sec_ptr, left),
        _section_query(b"diam3d", sec_ptr, left + 1),
        left_arc,
    )
    arc = left_arc
    total = 0.0
    for i in range(left + 1, n3d):
        next_arc = _section_query(b"arc3d", sec_ptr, i)
        next_diam = _section_query(b"diam3d", sec_ptr, i)
        if next_arc < right_arc:
            total += _scaled_frustum_volume(next_arc - arc, left_diam, next_diam)
            arc = next_arc
            left_diam = next_diam
        else:
            right_diam = _interpolate(arc, next_arc, left_diam, next_diam, right_arc)
            total += _scaled_frustum_volume(right_arc - arc, left_diam, right_diam)
            break
    return total * (math.pi / 12)


class Segment(SEGMENT_BASE):
    """A point at normalized position ``x`` in [0, 1] along a ``Section``.

    Obtain via ``sec(x)`` (e.g. ``soma(0.5)``). Range variables and
    mechanism properties are accessed as attributes: ``seg.v``,
    ``seg.diam``, ``seg.hh.gnabar``. A segment wrapper is a non-owning
    handle — the underlying state lives in the parent Section's node
    array, indexed by ``x``.
    """

    __module__ = NRN_CLASS_MODULE
    # The base has _sec, _x and _mech_cache, and sets them: Segment(section, x).
    __slots__ = ()

    # As NEURON's nrn.Segment: no pickle, copy or deepcopy (the default
    # copied the slots; copy.copy(seg) raised RecursionError).
    def __reduce_ex__(self, protocol):
        raise TypeError("cannot pickle 'nrn.Segment' object")

    @property
    def sec(self):
        return self._sec

    @property
    def x(self):
        return self._x

    def _v(self):
        """Membrane voltage through the optimized scalar range-var path."""
        sec = self._sec
        sec._check_alive()
        x = self._x
        if not 0.0 < x < 1.0 and _TREE_SET_UP[0] != EPOCH[0]:
            _ensure_topology(sec._sec, x)
        return _RANGEVAR_GET(_V_SYMBOL, sec._sec, x)

    if _FAST_RANGEVARS is None:  # otherwise the core reads v
        v = property(_v)

    def __repr__(self):
        # NEURON formats x with %g and names a dead host explicitly.
        if not _nrn_section_is_active(self._sec._sec):
            return "<segment of deleted section>"
        # Section.name, not a subclass's: ExtendedSection makes name a property.
        return f"{Section.name(self._sec)}({self.x:g})"

    @staticmethod
    def _node_key(x, nseg):
        # Real NEURON's node model: x=0 and x=1 are their own boundary nodes,
        # and every interior x in a segment shares that segment's center node.
        # Plain int(x*nseg) binning would merge x=0 into the first segment,
        # making s(0) == s(0.1). No FFI call.
        if x <= 0.0:
            return 0
        if x >= 1.0:
            return nseg + 1
        return 1 + min(int(x * nseg), nseg - 1)

    def __eq__(self, other):
        if not isinstance(other, Segment):
            return NotImplemented
        if self._sec._sec != other._sec._sec:
            return False
        nseg = self._sec.nseg
        return self._node_key(self._x, nseg) == self._node_key(other._x, nseg)

    def __hash__(self):
        # Consistent with __eq__ (same node key). Real NEURON's hash is x-based
        # and disagrees with its node-based eq (two equal interior segments can
        # hash differently). Deliberate departure; see docs/ARCHITECTURE.md.
        nseg = self._sec.nseg
        return hash((self._sec, self._node_key(self._x, nseg)))

    def __lt__(self, other):
        return _segment_order(self, other, operator.lt)

    def __le__(self, other):
        return _segment_order(self, other, operator.le)

    def __gt__(self, other):
        return _segment_order(self, other, operator.gt)

    def __ge__(self, other):
        return _segment_order(self, other, operator.ge)

    def ref(self, name):
        # Section.ref, not self._sec.ref: a Section subclass may define its own.
        return Section.ref(self._sec, name, self._x)

    def _check_rangevar_mechanism(self, name):
        """Raise AttributeError, with NEURON's message, if range variable
        *name* does not exist here because its mechanism is absent."""
        owner = _missing_owner(self._sec, name)
        if owner is _FAST_IMEM:
            raise AttributeError(f"{name} does not exist at {self!r}")
        if owner is not None:
            raise AttributeError(f"{name}, the mechanism does not exist at {self!r}")

    def area(self):
        """Return membrane area of this segment in um^2."""

        self._sec._check_alive()
        try:
            return float(_section_query(b"area", self._sec._sec, self._x))
        finally:
            # Not read-only: area() can recompute node areas, and a diameter of 0
            # is reset to 1e-6 there (nrnoc/treeset.cpp, nrn_area_ri_impl).
            _PACKAGE._bump_model_epoch()

    def ri(self):
        """Return axial resistance to parent segment in MOhm."""

        self._sec._check_alive()
        try:
            return float(_section_query(b"ri", self._sec._sec, self._x))
        finally:
            # Not read-only: ri() can recompute node areas, and a diameter of 0
            # is reset to 1e-6 there (nrnoc/treeset.cpp, nrn_area_ri_impl).
            _PACKAGE._bump_model_epoch()

    def volume(self):
        """Return volume of this segment in um^3, as NEURON computes it.

        0 at x = 0 and x = 1. With more than one 3D point, the frustums
        between the 3D points in this segment's share of the arc length, with
        the diameter interpolated at its edges. Otherwise a cylinder of the
        segment's diam over L/nseg. Port of seg_volume
        (9.0's nrnpy_nrn.cpp).
        """

        sec = self._sec
        # NEURON runs nrn_area_ri on a dirty section before it checks x. The
        # diam read does the same at every x: it recomputes the geometry and
        # clears recalc_area_, which sets a 3D section's L to its arc length,
        # and raises NEURON's error for a zero diam. HOC area() skips that at
        # the ends and prefixes the error.
        diam = self.diam
        x = self._x
        if not 0.0 < x < 1.0:
            return 0.0
        nseg = sec.nseg
        length = sec.L / nseg
        n3d = int(_section_query(b"n3d", sec._sec))
        if n3d > 1:
            iseg = int(x * nseg)
            return _frustum_volume(sec, n3d, iseg * length, (iseg + 1) * length)
        return math.pi * diam * diam / 4 * length

    def point_processes(self):
        """Return the point processes in this segment's node, in NEURON's order.

        NEURON lists them from the node's property list: newest first, and one
        moved with ``loc`` goes back to the head. MechanismType.pp_begin/pp_next
        walk this section's nodes in that order, one type at a time, so only
        this section's point processes are visited. nrn_segment_mechanisms,
        when the core has it, gives the order across types and limits the walk
        to the types in the node; without it the types follow MechanismType
        order. TODO(gap-75): at a node shared by two sections (the 1 end of a
        parent and the 0 end of a child) NEURON also lists the other section's
        point processes; only this section's are returned.
        """
        if not 0.0 < self._x < 1.0:  # NEURON finds the node with node_exact
            _ensure_topology(self._sec._sec, self._x)
        return self._point_processes()

    def _point_processes(self):
        # psection() lists them without setting the tree up, as NEURON does.
        mt, located = _point_process_types()
        node_order = self._node_symbol_names()
        if node_order is None:
            types = list(located)
        else:
            types = [name for name in dict.fromkeys(node_order) if name in located]
        if not types:
            return []

        nseg = self._sec.nseg
        my_key = Segment._node_key(self._x, nseg)
        found = {}
        sec_ptr = self._sec._sec
        # Locked from the push to the last pop: another thread's HOC call
        # between them would read or pop this section (gap-72).
        with HOC_LOCK:
            _nrn_section_push(sec_ptr)
            try:
                for name in types:
                    mt.select(located[name])
                    in_node = []
                    pp = mt.pp_begin()
                    while pp is not None:
                        loc = pp.get_loc()
                        # get_loc pushes the section. Only HOC's own pop ends
                        # core's tolerance of a pushed one.
                        _PACKAGE._n.pop_section()
                        if Segment._node_key(loc, nseg) == my_key:
                            in_node.append(pp)
                        pp = mt.pp_next()
                    if in_node:
                        found[name] = in_node
            finally:
                _nrn_section_pop()

        if node_order is None:
            return [pp for name in types for pp in found.get(name, ())]
        result = []
        queues = {name: iter(pps) for name, pps in found.items()}
        for name in node_order:
            queue = queues.get(name)
            if queue is not None:
                pp = next(queue, None)
                if pp is not None:
                    result.append(pp)
        # Anything the node list did not account for, kept rather than dropped.
        seen = {id(pp) for pp in result}
        result += [pp for pps in found.values() for pp in pps if id(pp) not in seen]
        return result

    def _node_symbol_names(self):
        """Names in this node's property list, or None without the C API."""
        if _nrn_segment_mechanisms is None:
            return None
        if not 0.0 < self._x < 1.0:
            try:
                _ensure_topology(self._sec._sec, self._x)
            except RuntimeError:  # a section loop
                return None
        count = _nrn_segment_mechanisms(self._sec._sec, self._x, None, 0)
        if count < 0:
            return None
        buf = (Symbol * max(count, 1))()
        count = _nrn_segment_mechanisms(self._sec._sec, self._x, buf, count)
        return [_nrn_symbol_name(buf[i]).decode("utf-8") for i in range(count)]

    def _zero_area_owner(self):
        """The section whose zero-area node NEURON uses for this x, or None.

        NEURON's seg.<mechanism> and `for mech in seg` look in
        node_exact(sec, x): at the 1 end the zero-area end node, at the 0 end
        of a root section the root node, at a child's 0 end the parent's node
        at the connection; connect(parent(x), 1) swaps the ends. Of the
        density mechanisms only extracellular is in a zero-area node.

        None for an interior x, and for a 0 end inside a parent's interior
        node: there the section's own mechanisms are used.
        TODO(gap-79): NEURON uses the parent's. A Mechanism on this segment
        can't read them: nrn_rangepointer looks in the section's own node and
        raises through ctypes when the mechanism is not there.

        Walked with orientation() and parentseg(), not nrn_segment_mechanisms:
        at x = 0 that runs setup_topology, which throws through ctypes on a
        section loop.
        """
        seg = self
        seen = set()
        while True:
            x = seg._x
            if 0.0 < x < 1.0:
                return None
            sec = seg._sec
            sec._check_alive()
            if _section_query(b"section_orientation", sec._sec):
                x = 1.0 - x
            if x >= 1.0:
                return sec
            if sec._sec in seen:  # a loop; NEURON raises when it sets it up
                return None
            seen.add(sec._sec)
            parent_ptr = _nrn_section_parent(sec._sec)
            if not parent_ptr:  # the root node
                return sec
            x = _section_query(b"parent_connection", sec._sec)
            seg = Segment(_section_for(parent_ptr), x)

    def _zero_area_mechanisms(self, owner):
        """The density mechanisms usable at a zero-area node of *owner*.

        Extracellular only, and only when this section has it too: a
        Mechanism on this segment reads through this section.
        """
        name = "extracellular"
        if not _has_mechanism(self._sec, name):
            return []
        if owner._sec != self._sec._sec and not _has_mechanism(owner, name):
            return []
        return [name]

    def node_index(self):
        """Return the internal NEURON node index for this segment.

        Mirrors real NEURON's ``segment.node_index()``. Indices are canonical
        once the tree is set up (after ``finitialize()``).

        Uses ``nrn_segment_node_index`` (gap-15, merged upstream as nrn#3810).
        """
        self._sec._check_alive()
        if not 0.0 < self._x < 1.0:
            _ensure_topology(self._sec._sec, self._x)
        return int(_nrn_segment_node_index(self._sec._sec, self._x))

    def __dir__(self):
        self._sec._check_alive()  # NEURON's dir() of a dead segment raises
        # Private helpers are not part of NEURON's segment; _ref_ names are.
        attrs = {
            name
            for name in super().__dir__()
            if not name.startswith("_") or name.startswith(("__", "_ref_"))
        }
        # Standard segment attributes always available
        attrs.update(
            [
                "v",
                "diam",
                "cm",
                "x",
                "sec",
                "area",
                "ri",
                "volume",
                "point_processes",
                "node_index",
                "ref",
            ]
        )
        # Add _ref_ variants for standard range vars
        attrs.update(["_ref_v", "_ref_diam", "_ref_cm"])
        if not 0.0 < self._x < 1.0:
            # At an end NEURON lists the node there (_zero_area_owner).
            end_mechanisms = _end_node_mechanisms(self)
            if end_mechanisms is not None:
                attrs.update(end_mechanisms)
                return sorted(attrs)
        # Add mechanism names inserted in this section

        try:
            n_inst = _PACKAGE._n
            for mech_name in _PACKAGE.mechanism._all_density_mechanism_names():
                if n_inst.ismembrane(mech_name, sec=self._sec):
                    attrs.add(mech_name)
            # Also add ion mechanisms
            for ion_name in ("na_ion", "k_ion", "ca_ion"):
                sym = _nrn_symbol(ion_name.encode("utf-8"))
                if sym and n_inst.ismembrane(ion_name, sec=self._sec):
                    attrs.add(ion_name)
        except Exception:
            pass
        return sorted(attrs)

    def _getattr(self, name):
        forget_moved_symbols()
        if name == "__name__":
            # help() probes this first; NEURON's help() of a dead segment raises.
            self._sec._check_alive()
            raise AttributeError(name)
        if name.startswith("_ref_"):
            name = name[5:]
            info = _rangevar_info(name)
            if info is None:
                raise _name_error(
                    AttributeError,
                    name,
                    f"_ref_{name} was not made to point to anything at {self!r}",
                )
            if not info[2]:  # NEURON finds a scalar's node now, then checks
                _ensure_rangevar_topology(self._sec._sec, self._x, name)
            self._check_rangevar_mechanism(name)
            if info[2]:
                return NrnRangeArrayRef(self._sec, self._x, name)
            if _is_pointer_rangevar(name, info[0]):
                # raises if unset; an opaque POINTER is its own reference
                value = _pointer_rangevar_get(self.sec, self.x, name)
                if isinstance(value, OpaquePointer):
                    return value
            return NrnRangeVarRef(self._sec, self._x, name)

        # Mechanism instance cache (Segment._mech_cache).
        # WHY: rebuilding a Mechanism wrapper on every seg.hh access
        # dominated seg.hh.gnabar cost (__init__ + __getattr__ symbol walk).
        # INVALIDATES: when nseg changed after the wrapper was made (it records
        # NSEG_GENERATION, and NEURON's old nodes are gone): a new wrapper for
        # the node now at x replaces it. Each Segment object has its own cache,
        # stored in a __slots__ entry, so no __dict__ is allocated per Segment.
        mech_cache = self._mech_cache
        if mech_cache is not None:
            cached_mech = mech_cache.get(name)
            if cached_mech is not None:
                sec_ptr = self._sec._sec
                if cached_mech._epoch == EPOCH[0]:
                    generation = NSEG_GENERATION.get(sec_ptr, 0)
                else:  # a HOC statement since may have changed nseg
                    generation = current_nseg_generation(sec_ptr)
                if cached_mech._nseg == generation:
                    return cached_mech

        info = _SYMBOL_INFO_CACHE.get(name)
        if info is None:
            info = _get_symbol_info(name)
        elif info[2]:
            info = _current_array_length(info)
        if info is None:
            raise _name_error(
                AttributeError, name, f"'nrn.Segment' object has no attribute '{name}'"
            )
        sym, sym_type, is_array, array_length = info
        if sym_type == TYPES.RANGEVAR:
            # Reading a range var on a HOC-deleted section dereferences a freed
            # node and SIGABRTs (node_index assertion, cabcode.cpp); raise
            # like real NEURON instead, which pays an equivalent check on this
            # access. A cached seg.<mechanism> wrapper checks on its own.
            sec = self._sec
            sec._check_alive()
            if is_array:
                # As NEURON's: the array object is handed out as is, and each
                # element access checks the mechanism.
                return _ArrayRangeVar(sec, self._x, name, array_length)
            if not 0.0 < self._x < 1.0 and _TREE_SET_UP[0] != EPOCH[0]:
                # Before the mechanism check, as NEURON finds the node first.
                _ensure_rangevar_topology(sec._sec, self._x, name)
            # diam, cm and i_cap (owner None) skip the method call.
            if _RANGEVAR_OWNER.get(name, _UNSET) is not None:
                self._check_rangevar_mechanism(name)
            if name == "diam":
                # Use nrn_segment_diam_get: it runs NEURON's per-section
                # recalc_area_, so a 3d-defined section reports the recomputed
                # diameter. The generic range-var read hits the raw pointer
                # and skips that recompute. A libnrniv without the upstream
                # fix returns the same value as the generic read, so this is
                # safe on any build.
                # NEURON defers the error for a non-positive diameter on a
                # stylized section: each read clamps exactly one offending
                # node (the last) and raises the literal "diam = 0." message,
                # repeating until none remain. On dev114 and later, including
                # the pinned dev121 (all include nrn#3854), the C getter
                # suppresses hoc_execerror, so this scan exists only to raise
                # the error real NEURON raises. It is gated on NEURON's dirty
                # flag (cleared by
                # finitialize or a global recompute), so hot loops pay one FFI
                # call: 0.77 us vs 58 us ungated at nseg=101. While a recompute is pending the
                # scan is O(nseg). The n3d guard limits it to nrn_area_ri's
                # no-3d-points branch: diam_from_list never raises, and a 3d
                # section's raw node value is not what the getter returns.
                if _DIAM_CHANGED.value and sec.n3d() <= 1:
                    nseg = sec.nseg
                    bad_x = None
                    for i in range(nseg):
                        x = (i + 0.5) / nseg
                        if _RANGEVAR_GET(sym, sec._sec, x) <= 0.0:
                            bad_x = x
                    if bad_x is not None:
                        _SEG_DIAM_SET(sec._sec, bad_x, 1e-6)
                        raise RuntimeError(
                            f"hoc_execerror: {sec.name()} diameter diam = 0. "
                            "Setting to 1e-6"
                        )
                return _SEG_DIAM_GET(sec._sec, _diam_node_x(sec, self._x))
            if _is_pointer_rangevar(name, sym):
                return _pointer_rangevar_get(sec, self._x, name)
            if _FAST_RANGEVARS is not None:
                _FAST_RANGEVARS[name] = sym
            return _RANGEVAR_GET(sym, sec._sec, self._x)
        if sym_type == TYPES.MECHANISM:

            # Only an inserted mechanism is accessible as seg.<mech>. Real
            # NEURON raises AttributeError otherwise; a wrapper for an
            # uninserted mechanism would let a later range-var read
            # dereference an absent prop and SIGABRT. Cold path only (warm
            # hits return the cached wrapper above). Check the per-section
            # inserted-mech cache first, then has_membrane (one ismembrane
            # call), which covers ions (the density-only cache omits them)
            # and the absent case.
            self._sec._check_alive()
            if not 0.0 < self._x < 1.0:
                # At an end NEURON uses the node there (_zero_area_owner).
                end_mechanisms = _end_node_mechanisms(self)
                if end_mechanisms is not None and name not in end_mechanisms:
                    raise AttributeError(
                        f"{name}, the mechanism does not exist at {self!r}"
                    )
            # A stale list (the epoch moved since it was built) is not rebuilt
            # here: that is one ismembrane call per density mechanism, and a
            # section or template made in between moves the epoch. One
            # has_membrane call answers. A list never built is built once.
            # A stale list (the epoch moved since it was built) is rebuilt
            # once, with the direct ismembrane, which does not move the
            # epoch: the next fresh segments then find it current.
            inserted = _current_inserted_mechs(self._sec)
            if inserted is None or inserted is _STALE:
                inserted = self._sec._get_inserted_mechs()
            if (
                inserted is None or name not in inserted
            ) and not self._sec.has_membrane(name):
                raise AttributeError(f"'{name}' is not inserted in {self._sec!r}.")
            mech = _PACKAGE.mechanism.Mechanism(self._uncached_copy(), name)
            if mech_cache is None:
                mech_cache = {}
                _segment_set_slot(self, "_mech_cache", mech_cache)
            mech_cache[name] = mech
            if _FAST_MECHANISMS is not None:
                _FAST_MECHANISMS.setdefault(name, {})
            return mech
        if TYPES.RANGEOBJ is None or sym_type == TYPES.RANGEOBJ:
            self._sec._check_alive()
            wrapped = _try_wrap_density_nmodlrandom(
                self._sec._sec, self._x, sym, sym_type
            )
            if wrapped is not None:
                return wrapped
        raise AttributeError(f"'{name}' is not a range variable on {self!r}.")

    def __contains__(self, obj):
        """True if obj.segment is this segment (an rxd node), as NEURON's."""
        if not hasattr(obj, "segment"):
            return False
        return self == obj.segment

    def __iter__(self):
        """Iterate over Mechanism objects for each inserted mechanism."""

        self._sec._check_alive()  # now, as NEURON does, not on the first next()
        # At an end NEURON uses the node there (_zero_area_owner).
        end_mechanisms = None if 0.0 < self._x < 1.0 else _end_node_mechanisms(self)

        def mechanisms():
            if end_mechanisms is not None:
                for mech_name in end_mechanisms:
                    yield _PACKAGE.mechanism.Mechanism(self._uncached_copy(), mech_name)
                return
            mech_cache = self._mech_cache
            generation = current_nseg_generation(self._sec._sec)
            for mech_name in self._mechanism_order():
                cached = mech_cache.get(mech_name) if mech_cache is not None else None
                if cached is not None and cached._nseg == generation:
                    yield cached
                    continue
                mech = _PACKAGE.mechanism.Mechanism(self._uncached_copy(), mech_name)
                if mech_cache is None:
                    mech_cache = {}
                    _segment_set_slot(self, "_mech_cache", mech_cache)
                mech_cache[mech_name] = mech
                yield mech

        return mechanisms()

    def _mechanism_order(self):
        """The inserted density mechanisms, in NEURON's order where known.

        NEURON yields them in the order of the node's property list, which
        depends on insertion order and differs between nodes after an nseg
        change; nrn_segment_mechanisms (when the core exports it) reports it.
        Without it, registration order. Not asked at an end: at x = 0 (or 1 on a
        swapped section) the core runs setup_topology, which throws through
        ctypes on a section loop.
        """
        inserted = self._sec._get_inserted_mechs()
        if (
            _nrn_segment_mechanisms is None
            or len(inserted) < 2
            or not 0.0 < self._x < 1.0
        ):
            return inserted
        buf = (Symbol * 64)()
        count = _nrn_segment_mechanisms(self._sec._sec, self._x, buf, 64)
        if count < 0 or count > 64:
            return inserted
        wanted = set(inserted)
        order = []
        for i in range(count):
            name = _nrn_symbol_name(buf[i]).decode("utf-8")
            if name in wanted:
                order.append(name)
                wanted.discard(name)
        return order + [name for name in inserted if name in wanted]

    def _uncached_copy(self):
        # A cached Mechanism holds a copy of this Segment, not the Segment
        # itself: Segment -> cache -> Mechanism -> Segment would be a
        # reference cycle, and the Section (a Python-owned one is deleted
        # when its last reference goes) would then live until the next
        # garbage collection. NEURON frees it at once (test_nrntest_thread
        # counted an extra cell's mechanism instances).
        return Segment(self._sec, self._x)

    def _assign_suffixed_pointer(self, suffixed, src_ref):
        for mech in self:
            mech_name = mech.name()
            if suffixed.endswith("_" + mech_name):
                try:
                    mech._assign_pointer(suffixed[: -len(mech_name) - 1], src_ref)
                except NameError as exc:
                    raise AttributeError(str(exc)) from None
                return
        raise AttributeError(
            f"'{suffixed}' is not a POINTER of a mechanism in {self!r}"
        )

    def _setattr(self, name, value):
        # seg._ref_p_t2 = ref wires mechanism t2's POINTER p, the same as
        # seg.t2._ref_p = ref (NEURON's test_pnt2).
        if name.startswith("_ref_") and isinstance(value, NrnRef):
            self._assign_suffixed_pointer(name[5:], value)
            return
        # Underscore-prefixed names are internal slots, never range vars.
        if name.startswith("_"):
            _segment_set_slot(self, name, value)
            return
        if value.__class__ is not float and value.__class__ is not int:
            value = _scalar_rangevar_double(name, value)
        # Before the write dereferences a possibly freed C node; see
        # __getattr__. NEURON checks first, with its own text for assignment.
        if not _nrn_section_is_active(self._sec._sec):
            raise ReferenceError(f"nrn.Segment {_DELETED_SECTION}")
        info = _get_symbol_info(name)
        if info is None:
            raise _name_error(
                AttributeError, name, f"'nrn.Segment' object has no attribute '{name}'"
            )
        sym, sym_type, is_array, array_length = info

        if sym_type != TYPES.RANGEVAR:
            # A RANDOM variable's symbol type is RANGEOBJ, never RANGEVAR, so
            # only other types are checked (RANGEOBJ stays unknown until a
            # wrapper validates a RANDOM symbol, api.TypeCodes.record_rangeobj).
            if TYPES.RANGEOBJ is None or sym_type == TYPES.RANGEOBJ:
                wrapped = _try_wrap_density_nmodlrandom(
                    self._sec._sec, self._x, sym, sym_type
                )
                if wrapped is not None:
                    raise ValueError(
                        f"NMODL RANDOM variable '{name}' is not assignable"
                    )
            raise AttributeError(f"'{name}' is not a range variable on {self!r}.")
        if not 0.0 < self._x < 1.0 and _TREE_SET_UP[0] != EPOCH[0] and not is_array:
            _ensure_rangevar_topology(self._sec._sec, self._x, name)  # as __getattr__
        self._check_rangevar_mechanism(name)
        if is_array:
            size = array_length
            # Accepting a whole sequence is a myneuron extension; a scalar
            # gets NEURON's own error.
            try:
                vals = list(value)
            except TypeError:
                raise IndexError(f"{name} needs an index for assignment") from None
            if len(vals) != size:
                raise ValueError(
                    f"cannot assign sequence of length {len(vals)} to "
                    f"array range variable '{name}' of length {size}"
                )
            wrapper = _ArrayRangeVar(self._sec, self._x, name, size)
            for i, v in enumerate(vals):
                wrapper[i] = v
            return
        if name == "diam":
            # Real NEURON accepts finite non-positive values, then clamps and
            # reports them on the next diam/geometry read. Keep that, but
            # reject NaN/Inf at the Python boundary.

            if not _is_finite(value):
                raise ValueError(f"diam must be a finite number, got {value!r}")
            sec = self._sec
            # On a core that calls Python, nrn_segment_diam_set first derives
            # the diameters of pending 3-D points (nrn#3921); a released core's
            # does not, as its legacy module does not.
            _SEG_DIAM_SET(sec._sec, _diam_node_x(sec, self._x), value)
            _diam_hoc_assign(
                sec, f"diam({self._x!r}) = {float(value)!r}", self._x, value
            )
            return
        if _is_pointer_rangevar(name, sym):
            _pointer_rangevar_set(self._sec, self._x, name, value)
            return
        try:
            _RANGEVAR_SET(sym, self._sec._sec, self._x, value)
        except ctypes.ArgumentError as exc:
            raise _refused_double(exc, ValueError) from None
        if _FAST_RANGEVARS is not None:
            _FAST_RANGEVARS[name] = sym

    if _FAST_RANGEVARS is None:  # otherwise the core's slots call them
        __getattr__ = _getattr
        __setattr__ = _setattr

    def cached(self):
        """Return a CachedSegment for this segment.

        It reads and writes through this segment after its own checks, so it
        is slower than seg.v. For tight loops, use seg.ptr('v')[0] (see its
        limits).
        """
        return CachedSegment(self)

    def ptr(self, name):
        """Return the raw ``c_double *`` for a scalar range variable.

        Reads and writes via the returned pointer (``p[0]``,
        ``p[0] = value``) bypass ``__getattr__``, symbol lookup, ctypes
        argument marshaling, and the ``nrn_rangevar_get`` FFI call. In
        a tight loop ``p[0]`` is about 20x faster than ``seg.<name>``
        (measured 23 ns vs 530 ns for v).

        The pointer is the variable's address at the time of the call, and
        nothing keeps track of it. NEURON keeps range variables in arrays
        that it grows, compacts and reorders, so the address can change on
        any of:
          - creating, deleting, connecting or disconnecting any section,
            changing any section's nseg (from Python or HOC), or changing
            the number of threads;
          - inserting or uninserting a mechanism anywhere: the arrays of
            that mechanism move;
          - the reordering NEURON does when it next sets the model up for a
            simulation (finitialize, fadvance, run) after such a change.
        A pointer taken after finitialize stays valid while the simulation
        runs, as long as the model does not change. After a change, reading
        through the old pointer returns another variable's value or freed
        memory, and writing through it corrupts memory: call ``seg.ptr(name)``
        again. ``seg.cached()`` and ``seg.<name>`` have no such limit.

        Only scalar range variables (HOC symbol type 310) are
        addressable. Array range vars and ion globals raise
        ``AttributeError``, and so does a variable whose mechanism is not
        in this section.
        """
        info = _get_symbol_info(name)
        if info is None:
            raise AttributeError(f"unknown range variable {name!r}")
        sym, sym_type, is_array, _ = info

        if sym_type != TYPES.RANGEVAR or is_array:
            raise AttributeError(
                f"{name!r} is not a scalar range variable "
                f"(type {sym_type}, array={is_array})"
            )
        # Acquiring the pointer looks up the C node: on a section deleted
        # through HOC, or where the variable's mechanism is absent, the core
        # throws through ctypes and the process aborts. Raise RuntimeError or
        # AttributeError first. So does a section loop at a 0 end. This
        # covers acquisition only; see above for how long the pointer stays
        # valid.
        self._sec._check_alive()
        _ensure_rangevar_topology(self._sec._sec, self._x, name)
        self._check_rangevar_mechanism(name)
        with HOC_LOCK:  # the pop must take this push (gap-72)
            _nrn_rangevar_push(sym, self._sec._sec, ctypes.c_double(self._x))
            raw = _nrn_double_ptr_pop()
        if not raw:
            raise RuntimeError(f"could not acquire pointer for {name!r}")
        return ctypes.cast(raw, ctypes.POINTER(ctypes.c_double))


# ── Section ───────────────────────────────────────────────────────────────────

# Times Section.nseg was changed, per section pointer. Mechanism wrappers and
# range-variable references record it at creation: NEURON frees the old nodes'
# storage when nseg changes, so they die. A dict read is far cheaper than
# reading nseg through the C API on every wrapper creation. An nseg change made
# from HOC code is counted by current_nseg_generation when a wrapper is made,
# and by sync_nseg when a range-variable reference is used.
# The core's fast path holds this dict (never rebind it, only change it).
NSEG_GENERATION = {}
if _FAST_RANGEVARS is not None:
    SEGMENT_BASE._fast_nseg_generation = NSEG_GENERATION

# nseg at each generation, per section pointer: NSEG_HISTORY[sec][g]. NEURON
# reuses some nodes when nseg changes (node_realloc in nrnoc/solve.cpp): all of
# them when it grows, and one per new node when it shrinks. A range-variable
# reference follows its node through those changes as NEURON's data handle
# does (follow_node), and dies only with it.
NSEG_HISTORY = {}


def _record_nseg(sec_ptr, nseg):
    history = NSEG_HISTORY.setdefault(sec_ptr, [])
    if not history or nseg != history[-1]:
        history.append(nseg)
        NSEG_GENERATION[sec_ptr] = len(history) - 1


def sync_nseg(sec_ptr):
    """Count an nseg change made since the last look, such as one from HOC code."""
    nseg = _nrn_nseg_get(sec_ptr)
    history = NSEG_HISTORY.get(sec_ptr)
    if history is None or nseg != history[-1]:
        _record_nseg(sec_ptr, nseg)


# Section pointer -> the model epoch at which current_nseg_generation last
# compared its nseg with NEURON's.
_NSEG_SYNCED_AT = {}


def current_nseg_generation(sec_ptr):
    """The section's generation, counting an nseg change made from HOC.

    NEURON's nseg is read (sync_nseg) only if the model epoch moved since the
    last read for this section: an nseg change from Python is recorded at
    once, and a HOC statement run from Python moves the epoch.
    """
    epoch = EPOCH[0] or _model_epoch()
    if _NSEG_SYNCED_AT.get(sec_ptr) != epoch:
        sync_nseg(sec_ptr)
        _NSEG_SYNCED_AT[sec_ptr] = epoch
    return NSEG_GENERATION.get(sec_ptr, 0)


def follow_node(sec_ptr, generation, x, reversed_):
    """Where the node at ``x`` when ``generation`` was current is now.

    Returns the x of that node's center now, or None if NEURON destroyed the
    node. ``x`` of 0 or 1 is an end node, which nseg changes keep. The index
    arithmetic is node_realloc's, in node (pnode) order, which is reversed
    for a section with orientation 1.
    """
    history = NSEG_HISTORY.get(sec_ptr)
    if not history or generation >= len(history) - 1 or x <= 0.0 or x >= 1.0:
        return x
    n = history[generation]
    index = min(int(n * x), n - 1)  # node_index
    if reversed_:
        index = n - index - 1
    for n1, n2 in zip(history[generation:-1], history[generation + 1 :]):
        if n1 < n2:
            # every old node is reused
            index = int(n2 * ((index + 0.5) / n1))
            continue
        # new node i2 reuses old node int(n1 * (i2 + .5) / n2); others die
        guess = int(index * n2 / n1)
        for i2 in (guess - 1, guess, guess + 1):
            if 0 <= i2 < n2 and int(n1 * ((i2 + 0.5) / n2)) == index:
                index = i2
                break
        else:
            return None
    n = history[-1]
    if reversed_:
        index = n - index - 1
    return (index + 0.5) / n


def _forget_nseg_history(sec_ptr):
    # NEURON allocates sections from a pool and reuses their addresses, so
    # the history must not outlive its section.
    NSEG_HISTORY.pop(sec_ptr, None)
    NSEG_GENERATION.pop(sec_ptr, None)
    _NSEG_SYNCED_AT.pop(sec_ptr, None)


# Weak references to the sections created from Python, by pointer: a wrapper made
# from a raw pointer (_from_ptr, e.g. sr.sec) can then tell a Python section from a
# HOC one.
PYTHON_SECTIONS = {}


def forget_section(ref):
    if PYTHON_SECTIONS.get(ref.key) is ref:
        del PYTHON_SECTIONS[ref.key]
        _forget_nseg_history(ref.key)


def _section_for(sec_ptr):
    """The section at *sec_ptr* as NEURON hands it to Python: the object it
    was created as (a subclass instance keeps its class and attributes), or
    a new wrapper for a section made in HOC.

    Every section handed out by pointer goes through here: n.cas() (and so
    get_segment(), get_loc(), NetCon.preseg()/postseg()), SectionList
    iteration, SectionRef and the tree methods. allsec() does the same lookup
    inline."""
    # The weak reference itself, as allsec() reads it: .get() costs more.
    ref = PYTHON_SECTIONS.get(sec_ptr)
    sec = ref() if ref is not None else None
    return sec if sec is not None else Section._from_ptr(sec_ptr)


def _hoc_statement_as_proc(name, statement):
    """The HOC proc *name* that runs *statement*: a call of it raises HOC's error."""
    n = _PACKAGE._n
    if not _nrn_symbol(name.encode()):
        n("proc %s() { %s }" % (name, statement))
    return getattr(n, name)


def _density_mechanism_symbol(name):
    """The symbol of the density mechanism *name*, for insert and uninsert.

    nrn_mechanism_insert takes any symbol's subtype for a mechanism type:
    another kind of symbol aborts the process or inserts some other mechanism.
    As in NEURON, only a density mechanism goes in or out (morphology and
    capacitance are in every section).
    """
    symbol = _nrn_symbol(name.encode("utf-8"))
    if (
        not symbol
        or int(_nrn_symbol_type(symbol)) != TYPES.MECHANISM
        or name in _EVERY_SECTION_MECHANISMS
    ):
        # NEURON's message, then the names that would work, so a typo is
        # easy to spot.
        available = ", ".join(sorted(_PACKAGE.mechanism._all_density_mechanism_names()))
        raise ValueError(
            f"argument not a density mechanism name. {name!r} is not "
            f"one; density mechanisms: {available or '(none)'}"
        )
    return symbol


def _segment_order(seg, other, compare):
    # NEURON's pyseg_richcmp orders by the segment's node address, and by
    # the object's own address against anything else. No public call gives
    # a node address; segments order by section address, then position.
    if isinstance(other, Segment):
        return compare(
            (seg._sec._sec, seg._node_key(seg._x, seg._sec.nseg)),
            (other._sec._sec, other._node_key(other._x, other._sec.nseg)),
        )
    return compare(seg._sec._sec, id(other))


def _section_order(sec, other, compare):
    # NEURON's pysec_richcmp: sections order by Section address; a
    # segment, HOC object or reference by its own address; anything else
    # is unordered.
    if isinstance(other, Section):
        return compare(sec._sec, other._sec)
    if isinstance(other, (Segment, NrnRef, _PACKAGE.object.Object, _utils.FuncWrapper)):
        return compare(sec._sec, id(other))
    return NotImplemented


if _FAST_RANGEVARS is not None:  # only exact instances take the core's fast path
    SEGMENT_BASE._fast_segment_class = Segment


class _SectionSegments:
    """for seg in sec, as NEURON's iterator: nseg is read again at each step,
    so rediscretizing changes the remaining midpoints and the stopping point,
    and an exhausted iterator yields again once nseg grows. A section deleted
    meanwhile ends the loop."""

    __slots__ = ("_section", "_i")

    def __init__(self, section):
        self._section = section
        self._i = 0

    def __iter__(self):
        return self

    def __next__(self):
        sec_ptr = self._section._sec
        if not _nrn_section_is_active(sec_ptr):
            raise StopIteration
        nseg = _nrn_nseg_get(sec_ptr)
        if self._i >= nseg:
            raise StopIteration
        self._i += 1
        return Segment(self._section, (self._i - 0.5) / nseg)


class Section(SECTION_BASE):
    """A NEURON section — a continuous unbranched cable, the unit of topology.

    Create with ``n.Section('soma')`` (canonical) or ``Section('soma')``.
    Connect with ``dend.connect(soma(1), 0)``. Set section-level
    properties via attributes (``soma.L = 20``, ``soma.diam = 20``,
    ``soma.nseg = 5``, ``soma.Ra = 100``). Insert density mechanisms
    with ``soma.insert('hh')``. Iterate segments by index or via
    ``for seg in soma: ...``; access a specific segment with
    ``soma(x)`` for ``x`` in [0, 1].
    """

    # __weakref__: dependents hold Python-owned Sections weakly, so the last
    # owning wrapper still controls native lifetime. Dependents hold non-owning
    # HOC/template wrappers strongly (gap-50).
    __module__ = NRN_CLASS_MODULE
    # The base has the other slots, and makes the section. When an object goes
    # so does its section; a wrapper drops the reference _from_ptr took.
    __slots__ = ("__weakref__",)

    def __init__(self, name=None, cell=None):
        prefix = ""
        if cell is not None:
            if isinstance(cell, str):
                raise TypeError("cell must be an object, not a string")
            prefix = str(cell) + "."  # not format(): as NEURON
            if not prefix.isascii():
                raise TypeError("cell name contains non ascii character")
            cell = weakref.ref(cell)
            if not CALLS_PYTHON:
                cell = ("" if name is None else prefix, cell)
        if name is not None:
            if name.__class__ is not str and not isinstance(name, str):
                raise TypeError(f"argument 1 must be str, not {type(name).__name__}")
            if "\x00" in name:
                raise ValueError("embedded null character")
            # Fixed here, as in NEURON.
            name = f"{prefix}{name}".encode("utf-8")
        # Does nothing for an object that has its section, as NPySecObj_init.
        super().__init__(name, cell)
        ptr = self._sec
        ref = PYTHON_SECTIONS.get(ptr)
        if ref is None or ref() is not self:
            _forget_nseg_history(ptr)  # a HOC section's that was here
            PYTHON_SECTIONS[ptr] = weakref.KeyedRef(self, forget_section, ptr)

    @classmethod
    def _from_ptr(cls, sec_ptr):
        """Create a non-owning Section wrapper from a raw C Section* pointer.

        *sec_ptr* must be a section whose struct is not freed: one in the
        model, or one something holds a reference to.

        Hot path: writes only `_sec` and `_pynative`. The lazy slots `_cell`
        and `_inserted_mechs` stay unset, and their readers (`Section.cell()`,
        `Section._get_inserted_mechs()`) treat an unset slot as None. This
        saves two slot writes per yielded section on the allsec() hot path,
        about a third of `_from_ptr`'s cost.

        CAUTION: any caller that adds code to _from_ptr paths must use
        object.__getattribute__ for _cell and _inserted_mechs; normal
        attribute access recurses via __getattr__.
        """
        if not sec_ptr or sec_ptr.__class__ in (str, bytes):
            # nrn_section_ref(NULL) or a string pointer would crash the process.
            raise TypeError(f"not a section pointer: {sec_ptr!r}")
        obj = cls.__new__(cls)
        # The wrapper holds the struct, as NEURON's does (section_ref in
        # 9.0's nrnpy_nrn.cpp). Without it, a HOC delete_section freed the struct
        # and about 1000 creations later a new section took its slot: the
        # wrapper then read and wrote that section, and delete_section(sec=)
        # deleted it. Taken before the slots are set, so __del__ drops only
        # a reference that was taken.
        _nrn_section_ref(sec_ptr)
        _object_setattr = object.__setattr__
        _object_setattr(obj, "_sec", sec_ptr)
        _object_setattr(obj, "_pynative", False)
        return obj

    def _check_alive(self):
        """Raise NEURON's ``ReferenceError`` if this section has been deleted.

        Every access through a deleted section must check first: the core
        would read freed Section and node storage and abort.
        """
        if not _nrn_section_is_active(self._sec):
            raise ReferenceError(_DELETED_SECTION)

    def __eq__(self, other):
        if not isinstance(other, Section):
            return NotImplemented
        return self._sec == other._sec

    def __hash__(self):
        return hash(self._sec)

    def __lt__(self, other):
        return _section_order(self, other, operator.lt)

    def __le__(self, other):
        return _section_order(self, other, operator.le)

    def __gt__(self, other):
        return _section_order(self, other, operator.gt)

    def __ge__(self, other):
        return _section_order(self, other, operator.ge)

    # As NEURON's nrn.Section: no copying or pickling. The default would copy
    # the slots, the section pointer and _pynative among them, and each copy's
    # __del__ would then delete the section and drop a reference only the
    # original holds (a double free in NEURON's section pool). pc.submit
    # pickles its arguments, so a Section argument is refused there too. A
    # subclass that defines __reduce__ says how to rebuild itself.
    def __copy__(self):
        raise TypeError("cannot pickle 'nrn.Section' object")

    def __deepcopy__(self, memo):
        raise TypeError("cannot pickle 'nrn.Section' object")

    def __reduce_ex__(self, protocol):
        if type(self).__reduce__ is not object.__reduce__:
            return self.__reduce__()
        raise TypeError("cannot pickle 'nrn.Section' object")

    def __setattr__(self, name, value):
        # Private names and known @property setters go to super();
        # everything else broadcasts across segments, matching real NEURON's
        # `sec.gnabar_hh = ...` semantics.
        if name.startswith("_") or name in {"nseg", "L", "Ra", "rallbranch"}:
            if name == "_sec" and (not value or value.__class__ in (str, bytes)):
                # Every later access would pass it to the core and crash.
                raise TypeError(f"not a section pointer: {value!r}")
            try:
                super().__setattr__(name, value)
            except AttributeError:
                if type(self) is Section and not hasattr(Section, name):
                    raise AttributeError(
                        f"'nrn.Section' object has no attribute '{name}'"
                    ) from None
                raise
            return
        if type(self) is Section:
            # As NEURON's section_setattro: only a range variable broadcasts.
            info = _SYMBOL_INFO_CACHE.get(name)
            if info is None:
                info = _get_symbol_info(name)
            if info is None or info[1] != TYPES.RANGEVAR:
                self._check_alive()  # a deleted section's error comes first
                if not name.isascii():
                    raise TypeError("attribute name must be a string")
                cls_attr = getattr(Section, name, _UNSET)
                if isinstance(cls_attr, property):
                    super().__setattr__(name, value)
                    return
                if cls_attr is not _UNSET:
                    raise AttributeError(
                        f"'nrn.Section' object attribute '{name}' is read-only"
                    )
                raise AttributeError(f"'nrn.Section' object has no attribute '{name}'")
        if value.__class__ is not float and value.__class__ is not int:
            value = _scalar_rangevar_double(name, value)
        if name == "diam":
            # One HOC broadcast reaches nrn_diam_change once instead of nseg
            # round trips (2.25 ms at nseg=101). Raw per-node stores come
            # first, as in the per-segment path: under pt3dconst(1) they are
            # the only write that lands.

            if not _is_finite(value):
                raise ValueError(f"diam must be a finite number, got {value!r}")
            nseg = self.nseg
            for i in range(nseg):
                _SEG_DIAM_SET(self._sec, (i + 0.5) / nseg, value)
            _diam_hoc_assign(self, f"diam = {float(value)!r}", "0:1", value)
        elif name == "v":
            # NEURON sets v on every node of the section, both ends included;
            # a child's x=0 node is its parent's node and changes too.
            for seg in self.allseg():
                seg.v = value
        elif type(self) is not Section and not _is_rangevar_name(name):
            # A subclass (neuron.sections.ExtendedSection) keeps its own
            # attributes and properties; NEURON broadcasts range variables only.
            object.__setattr__(self, name, value)
        else:
            try:
                for seg in self:
                    setattr(seg, name, value)
            except AttributeError:
                if _get_symbol_info(name) is None:
                    raise AttributeError(
                        f"'nrn.Section' object has no attribute '{name}'"
                    ) from None
                raise
            except IndexError as exc:
                # NEURON's text for a section-level array without a subscript.
                if exc.args == (f"{name} needs an index for assignment",):
                    raise IndexError("missing index") from None
                raise

    def __dir__(self):
        attrs = set(object.__dir__(self))
        attrs.update(
            [
                "L",
                "Ra",
                "nseg",
                "rallbranch",
                "allseg",
                "arc3d",
                "cell",
                "children",
                "connect",
                "diam3d",
                "disconnect",
                "has_membrane",
                "hname",
                "hoc_internal_name",
                "insert",
                "is_pysec",
                "n3d",
                "name",
                "orientation",
                "parentseg",
                "psection",
                "pt3dadd",
                "pt3dchange",
                "pt3dclear",
                "pt3dinsert",
                "pt3dremove",
                "pt3dstyle",
                "push",
                "rallbranch",
                "same",
                "spine3d",
                "subtree",
                "trueparentseg",
                "uninsert",
                "wholetree",
                "x3d",
                "y3d",
                "z3d",
            ]
        )
        return sorted(attrs)

    def __getattr__(self, name):
        # An unset _sec slot (partial __init__) would recurse through
        # _check_alive; raise AttributeError directly instead.
        try:
            object.__getattribute__(self, "_sec")
        except AttributeError:
            raise AttributeError(name) from None
        self._check_alive()
        # As NEURON's section_getattro: only a range variable (diam, cm, v,
        # i_cap, a mechanism's RANGE names) reads through to the middle
        # segment. Not area, ri, x, _ref_*, sec, point_processes or a
        # mechanism name (sec.hh).
        info = _SYMBOL_INFO_CACHE.get(name)
        if info is None:
            info = _get_symbol_info(name)
        if info is None or info[1] != TYPES.RANGEVAR:
            if name == "__dict__":
                return dict.fromkeys(("L", "Ra", "nseg", "rallbranch"))
            if not name.isascii():
                raise TypeError("attribute name must be a string")
            raise AttributeError(f"'nrn.Section' object has no attribute '{name}'")
        value = getattr(self(0.5), name)
        if value.__class__ is _ArrayRangeVar:
            # Same layout: re-class the fresh wrapper instead of rebuilding it.
            value.__class__ = _SectionArrayRangeVar
        return value

    def __contains__(self, obj):
        """True if obj.sec is this section (a Segment, an rxd node), as NEURON's."""
        if not hasattr(obj, "sec"):
            return False
        return self == obj.sec

    def __iter__(self):
        self._check_alive()  # now, as NEURON does, not on the first next()
        if _FAST_RANGEVARS is not None:
            return self._segments()  # the core's iterator, the same loop
        return _SectionSegments(self)

    def allseg(self):
        """Iterate all segments including endpoints at x=0 and x=1."""
        self._check_alive()  # now, as NEURON does, not on the first next()

        def segments():
            yield self(0)
            yield from self
            yield self(1)

        return segments()

    def insert(self, mechanism):

        # Direct-C path (no sec= push): inserting into a section freed through
        # HOC SIGSEGVs the core.
        self._check_alive()
        if isinstance(mechanism, _PACKAGE.mechanism.DensityMechanism):
            mechanism = mechanism.name
        elif not isinstance(mechanism, (str, bytes)):
            # As NEURON's: an object with an insert method inserts itself
            # (neuron.DensityMechanism, which the neuron package's h.hh returns).
            insert = getattr(mechanism, "insert", None)
            if insert is None:
                raise TypeError(
                    "insert argument must be either a string or an object with "
                    "an insert method"
                )
            insert(self)
            return self
        if isinstance(mechanism, bytes):
            mechanism = mechanism.decode("utf-8")
        mech_symbol = _density_mechanism_symbol(mechanism)
        # In a callback core's data may be frozen. HOC returns the error; through
        # ctypes it would unwind the callback's Python frames (gap-89).
        if in_hoc_call():
            _hoc_statement_as_proc(f"_mn_insert_{mechanism}", f"insert {mechanism}")(
                sec=self
            )
        else:
            _nrn_mechanism_insert(self._sec, mech_symbol)

        _bump_model_epoch()
        self._inserted_mechs = None
        return self  # as NEURON's, so inserts chain

    def uninsert(self, mechanism):
        """Remove a mechanism; nothing to do if it is not inserted.

        Through the HOC ``uninsert`` statement: ``neuronapi.h`` has no
        ``nrn_mechanism_uninsert`` (gap-8).
        """
        self._check_alive()
        if isinstance(mechanism, _PACKAGE.mechanism.DensityMechanism):
            mechanism = mechanism.name
        elif not isinstance(mechanism, str):
            raise TypeError(f"argument 1 must be str, not {type(mechanism).__name__}")
        _density_mechanism_symbol(mechanism)
        _PACKAGE._n(f"uninsert {mechanism}", sec=self)
        self._inserted_mechs = None
        return self  # as NEURON's

    def _get_inserted_mechs(self):
        """Return the cached list of mechanism names inserted on this section.

        WHY: avoids an ismembrane call per mechanism per Segment iteration
        (N x M HOC round trips, M = registered mechanisms).
        INVALIDATES: local insert()/uninsert()/nseg-set reset the slot to None;
        HOC-side mutations and inserts through another wrapper of the same
        native Section* are caught by the global model epoch. The cache is
        per Section wrapper.

        The slot can be unset on wrappers from `_from_ptr`. Read it with
        object.__getattribute__ (see Section.cell()) and treat unset as None.
        """
        try:
            cached = object.__getattribute__(self, "_inserted_mechs")
        except AttributeError:
            cached = None

        epoch = EPOCH[0] or _model_epoch()
        if cached is not None:
            # HOC-side inserts and inserts through another wrapper skip local
            # invalidation, so also require the current model epoch (see
            # api.EPOCH). An unset epoch slot counts as stale.
            try:
                built = object.__getattribute__(self, "_inserted_mechs_epoch")
            except AttributeError:
                built = -1
            if built == epoch:
                return cached

        # The direct ismembrane: a HOC call through n would move the epoch and
        # make the list stale at once.
        inserted = [
            name
            for name in _PACKAGE.mechanism._all_density_mechanism_names()
            if _ismembrane(self._sec, name)
        ]
        self._inserted_mechs = inserted
        # Recorded after the calls: a call that bumps the epoch would make the
        # cache stale as soon as it is built.
        self._inserted_mechs_epoch = EPOCH[0] or _model_epoch()
        return inserted

    def has_membrane(self, mechanism):
        """Return True if *mechanism* is inserted in this section."""

        if isinstance(mechanism, str) and mechanism.isascii():
            # Direct, as _missing_owner: no HOC call, so the epoch stays.
            return _ismembrane(self._sec, mechanism)
        return bool(_PACKAGE._n.ismembrane(mechanism, sec=self))

    def name(self):
        self._check_alive()

        # The API's own secname: HOC's secname() adds a call round trip and
        # returns the same string.
        raw = _nrn_secname(self._sec).decode("utf-8")
        if raw.startswith("_pysec."):
            raw = raw[7:]
        if CALLS_PYTHON:
            return raw  # core's name already has the cell, as in NEURON
        entry = _cell_entry(self)
        return raw if entry is None else entry[0] + raw

    def __repr__(self):
        # name() is already cell-qualified; prefixing again would give
        # "<cell>.<cell>.<raw>". Real NEURON prints the single qualified name.
        if not _nrn_section_is_active(self._sec):
            return "<deleted section>"
        # Section.name, not self.name: a subclass may define its own `name`
        # (neuron.sections.ExtendedSection makes it a property).
        return Section.name(self)

    def __call__(self, x=0.5):
        self._check_alive()
        if 0 <= x <= 1:
            return Segment(self, float(x))
        else:
            raise ValueError("segment position range is 0 <= x <= 1")

    def ref(self, name, x=0.5):
        if not (0 <= x <= 1):
            raise ValueError("segment position range is 0 <= x <= 1")
        if _rangevar_info(name) is None:
            raise NameError(f"{name} is not a range variable")
        return NrnRangeVarRef(self, x, name)

    @property
    def nseg(self):
        self._check_alive()

        return _nrn_nseg_get(self._sec)

    @nseg.setter
    def nseg(self, value):
        if value.__class__ is not int and isinstance(value, numbers.Integral):
            value = int(value)  # may run user code: before the section is touched
        self._check_alive()
        # NSEG_MAX is 32767 (cabcode.cpp). The C API prints a warning and
        # coerces to 1 on overflow; raise instead, as real NEURON's wrapper does.
        # numbers.Integral, not int: numpy integer types are not int subclasses
        # but real NEURON accepts them. bool is Integral too (True -> nseg 1),
        # as in real NEURON.

        if not isinstance(value, numbers.Integral) or value <= 0 or value > 32767:
            raise ValueError(
                f"nseg must be an integer in range 1 to 32767, got {value!r}"
            )

        sync_nseg(self._sec)  # an nseg change made from HOC code comes first
        if in_hoc_call():  # as in insert()
            _hoc_statement_as_proc("_mn_set_nseg", "nseg = $1")(int(value), sec=self)
        else:
            _nrn_nseg_set(self._sec, int(value))
        # nrn_nseg_set flips diam_changed, which CachedSegment watches
        # (api.get_topology_version). New nodes free the old mechanism storage,
        # so Mechanism wrappers and range-variable references must revalidate.
        # Setting the same nseg changes nothing in NEURON.
        _record_nseg(self._sec, int(value))

        _bump_model_epoch()
        self._inserted_mechs = None

    def cell(self):
        """Return the cell object owning this section, or None.

        A section created inside a HOC template has no Python cell; its owner
        is the template object, which core reports.
        """
        self._check_alive()
        cell = _python_cell(self)
        if cell is None:
            cell = _PACKAGE._n.section_owner(sec=self)
        return cell

    @property
    def L(self):
        self._check_alive()

        return _nrn_section_length_get(self._sec)

    @L.setter
    def L(self, value):
        value = _to_double(value, ValueError)
        self._check_alive()

        # NaN passes `value <= 0` and inf passes `> 0`; reject both explicitly.

        if not _is_finite(value) or value <= 0:
            raise ValueError("L must be > 0.")
        _nrn_section_length_set(self._sec, value)

    @property
    def Ra(self):
        self._check_alive()

        return _nrn_section_Ra_get(self._sec)

    @Ra.setter
    def Ra(self, value):
        value = _to_double(value, ValueError)
        self._check_alive()
        # The C API (neuronapi.cpp:115) accepts anything, with a pending
        # "ensure val > 0" note. Real NEURON's wrapper validates
        # (9.0's nrnpy_nrn.cpp); match it. NaN and inf need explicit checks.

        if not _is_finite(value) or value <= 0:
            raise ValueError("Ra must be > 0.")

        _nrn_section_Ra_set(self._sec, value)

    @property
    def rallbranch(self):
        self._check_alive()

        return _nrn_section_rallbranch_get(self._sec)

    @rallbranch.setter
    def rallbranch(self, value):
        value = _to_double(value, ValueError)
        self._check_alive()

        _nrn_section_rallbranch_set(self._sec, value)

    def push(self):
        """Push this section onto the section stack."""
        self._check_alive()
        # Not between another thread's push and pop, which would pop this one.
        with HOC_LOCK:
            _nrn_section_push(self._sec)
        return self

    def same(self, other):
        """Return True if *other* refers to the same underlying section."""
        self._check_alive()
        if not isinstance(other, Section):
            return False
        return self._sec == other._sec

    def hoc_internal_name(self):
        """Return the internal HOC name (e.g. '__nrnsec_0x...')."""
        self._check_alive()

        addr = ctypes.cast(self._sec, ctypes.c_void_p).value
        return f"__nrnsec_0x{addr:x}"

    def is_pysec(self):
        """Return True if this section was created from Python.

        Known discrepancy with real NEURON: NEURON's C core marks sections
        created via its Python nrn.Section() with a '_pysec.' name prefix.
        nrn_section_new (the C API we use) does not set this prefix, so a
        section that myneuron created looks identical to a HOC-created one
        from the C side. We track ownership in Python via _pynative instead.
        Consequence: real NEURON's is_pysec() returns False for myneuron
        sections, but myneuron's returns True.
        """
        self._check_alive()
        return self._pynative

    def connect(self, parent, parent_x=_CONNECT_ARG_UNSET, child_x=0):
        """Connect this section to a parent section.

        Args:
            parent: parent Section, or a Segment whose .sec / .x are used
            parent_x: position on a parent Section (default 1). When *parent*
                is a Segment, this positional argument is the child end.
            child_x: position on this section (default 0)
        """
        if isinstance(parent, Segment):
            # Real NEURON overloads connect(parent_segment, child_end): the
            # segment supplies parent_x and the second positional argument is
            # reinterpreted as child_x (9.0's nrnpy_nrn.cpp::NPySecObj_connect).
            if parent_x is not _CONNECT_ARG_UNSET:
                child_x = parent_x
            parent_x = parent.x
            parent = parent.sec
        elif parent_x is _CONNECT_ARG_UNSET:
            parent_x = 1
        if not isinstance(parent, Section):
            raise TypeError(f"parent must be a Section, got {type(parent).__name__}")
        # Direct-C path: a deleted endpoint lets a C++ exception cross ctypes
        # and abort.
        self._check_alive()
        parent._check_alive()
        # Validate first: nrn_section_connect throws on out-of-range positions,
        # and the uncaught C++ exception std::terminate's the process.
        if not (0.0 <= float(parent_x) <= 1.0):
            raise ValueError(f"parent_x must be in [0, 1], got {parent_x}")
        if float(child_x) not in (0.0, 1.0):
            raise ValueError(f"child connection end must be 0 or 1, got {child_x}")
        # So does a connection that closes a loop, itself included.
        if _connect_forms_loop(self._sec, parent._sec, float(parent_x)):
            # NEURON disconnects the child before it raises (cabcode.cpp:1606).
            # TODO(gap-82): it has also reversed the child to child_x by then;
            # a nothrow connect in the C API would leave all of it to NEURON.
            if _nrn_section_parent(self._sec):
                _PACKAGE._n.disconnect(sec=self)
            raise RuntimeError(
                f"hoc_execerror: {Section.name(self)} connection will form loop"
            )

        _nrn_section_connect(self._sec, child_x, parent._sec, parent_x)
        return self

    def parentseg(self):
        """Return the parent Segment, or None if this is a root section."""
        self._check_alive()

        n = _PACKAGE._n
        parent_ptr = _nrn_section_parent(self._sec)
        if not parent_ptr:
            return None
        parent_sec = _section_for(parent_ptr)
        parent_x = n.parent_connection(sec=self)
        return Segment(parent_sec, parent_x)

    def children(self):
        """Return a list of child Sections.

        Walks the section's own child/sibling chain via the nrn#3835 accessors,
        in the same order as real NEURON's ``SectionRef.child[i]``.
        """
        self._check_alive()
        result = []
        child_ptr = _nrn_section_child(self._sec)
        while child_ptr:
            result.append(_section_for(child_ptr))
            child_ptr = _nrn_section_sibling(child_ptr)
        return result

    def orientation(self):
        """Return which end (0 or 1) of this section is connected to parent."""

        self._check_alive()
        return _section_query(b"section_orientation", self._sec)

    def subtree(self):
        """Return a list of this section and all its descendants.

        NEURON's order: depth first, each section before its children.
        Walked with a stack, not recursion, so a deep tree (a 1200-section
        chain) does not hit Python's recursion limit.
        """
        self._check_alive()
        result = []
        seen = set()
        stack = [self]
        while stack:
            sec = stack.pop()
            if sec._sec in seen:
                # connect() allows a loop. NEURON's subtree() then overflows
                # the C stack; without this check the walk would never end.
                raise RuntimeError(f"{sec} is in a loop of connected sections")
            seen.add(sec._sec)
            result.append(sec)
            stack.extend(reversed(sec.children()))
        return result

    def wholetree(self):
        """Return all sections in the tree containing this section."""
        self._check_alive()
        root = self
        while True:
            ps = root.parentseg()
            if ps is None:
                break
            root = ps.sec
        return root.subtree()

    def hname(self):
        """Return the HOC section name.

        In myneuron this delegates to name(); both read the section's HOC name
        directly (no HOC call). Real NEURON distinguishes the two for
        cell-owned sections (hname() returns the qualified HOC path); this
        implementation does not. Section.name, as in __repr__: a subclass's
        own `name` does not change it.
        """
        return Section.name(self)

    def trueparentseg(self):
        """Return the true parent Segment, or None.

        The "true parent" is the nearest ancestor that actually drives current
        into this section through the cable equation. A section connected at its
        parent's oriented beginning (``nrn_at_beginning``: the child's
        connection position equals the parent's orientation) sits at the same
        node as that parent, so it is skipped and the walk continues up the
        ancestors until a section is not at its parent's beginning; that
        parent is the true parent. This is an ancestor walk, not a single hop
        (it differs from one hop for multi-level 0-connected chains). Mirrors
        NEURON's ``nrn_trueparent`` / ``pysec_trueparentseg`` (cabcode.cpp:1577,
        9.0's nrnpy_nrn.cpp).
        """
        self._check_alive()

        n = _PACKAGE._n
        sec = self
        ps = sec.parentseg()
        psec = ps.sec if ps is not None else None
        while psec is not None:
            # at_beginning(sec) = connection_position(sec) == orientation(parent)
            if n.parent_connection(sec=sec) == psec.orientation():
                sec = psec
                ps = sec.parentseg()
                psec = ps.sec if ps is not None else None
            else:
                break
        if psec is None:
            return None
        return Segment(psec, n.parent_connection(sec=sec))

    def spine3d(self, i):
        """Return spine flag at 3D point i."""

        return int(_PACKAGE._n.spine3d(i, sec=self))

    def disconnect(self):
        """Disconnect this section from its parent."""

        _PACKAGE._n.disconnect(sec=self)

    def psection(self):
        """Return a dict of section properties matching real NEURON's format."""
        self._check_alive()
        segments = list(self)

        # morphology
        diam_list = [seg.diam for seg in segments]
        pts3d = []
        n3 = int(self.n3d())
        for i in range(n3):
            pts3d.append((self.x3d(i), self.y3d(i), self.z3d(i), self.diam3d(i)))

        # density_mechs and ions via mechanism iteration
        density_mechs = {}
        ions = {}
        for seg in segments:
            for mech in seg:
                mname = mech.name()
                if mech.is_ion():
                    ion_key = mname.split("_")[0]  # na_ion → na
                    if ion_key not in ions:
                        ions[ion_key] = {}
                    for var in mech:
                        vname = var.name()
                        ions[ion_key].setdefault(vname, []).append(var[0])
                else:
                    if mname not in density_mechs:
                        density_mechs[mname] = {}
                    for var in mech:
                        # Strip mechanism suffix for display name
                        vname = var.name()
                        suffix = f"_{mname}"
                        short = (
                            vname[: -len(suffix)] if vname.endswith(suffix) else vname
                        )
                        density_mechs[mname].setdefault(short, []).append(var[0])
                    suffix = f"_{mname}"
                    for full_name in mech._random_names():
                        short = full_name[: -len(suffix)]
                        density_mechs[mname].setdefault(short, []).append(
                            getattr(mech, short)
                        )

        # point processes
        # TODO(gap-46): point-process scan is O(segments × PP-classes); consider batching or lazy-loading.
        pp_dict = {}
        for seg in self.allseg():
            for pp in seg._point_processes():
                class_name = type(pp).__name__
                if class_name not in pp_dict:
                    pp_dict[class_name] = []
                pp_dict[class_name].append(pp)

        # parent/trueparent
        ps = self.parentseg()
        tps = self.trueparentseg()

        return {
            "point_processes": pp_dict,
            "density_mechs": density_mechs,
            "ions": ions,
            "morphology": {
                "L": self.L,
                "diam": diam_list,
                "pts3d": pts3d,
                "parent": ps,
                "trueparent": tps,
            },
            "nseg": self.nseg,
            "Ra": self.Ra,
            "cm": [seg.cm for seg in segments],
            "regions": set(),
            "species": set(),
            "name": Section.name(self),
            "hoc_internal_name": self.hoc_internal_name(),
            "cell": self.cell(),
        }

    def arc3d(self, i):
        return _PACKAGE._n.arc3d(i, sec=self)

    def diam3d(self, i):
        return _PACKAGE._n.diam3d(i, sec=self)

    def pt3dadd(self, x, y, z, d):
        _PACKAGE._n.pt3dadd(x, y, z, d, sec=self)

    def pt3dchange(self, i, *args):
        return _PACKAGE._n.pt3dchange(i, *args, sec=self)

    def pt3dclear(self, *args):
        return int(_PACKAGE._n.pt3dclear(*args, sec=self))

    def pt3dinsert(self, i, x, y, z, d):
        _PACKAGE._n.pt3dinsert(i, x, y, z, d, sec=self)

    def pt3dremove(self, i):
        """remove the ith point"""

        _PACKAGE._n.pt3dremove(i, sec=self)

    def pt3dstyle(self, *args):
        return bool(_PACKAGE._n.pt3dstyle(*args, sec=self))

    def n3d(self):
        self._check_alive()
        return int(_section_query(b"n3d", self._sec))

    def x3d(self, i):
        return _PACKAGE._n.x3d(i, sec=self)

    def y3d(self, i):
        return _PACKAGE._n.y3d(i, sec=self)

    def z3d(self, i):
        return _PACKAGE._n.z3d(i, sec=self)


# ── CachedSegment ─────────────────────────────────────────────────────────────


class CachedSegment:
    """Attribute access to a Segment's variables, bound to the
    discretization and model setup it was made in.

    ``cs.v``, ``cs.gnabar_hh = 0.13`` and ``cs.area()`` go through the
    Segment, so each read and write sees NEURON's current storage and is
    checked as ``seg.v`` is (a deleted section raises ReferenceError, an absent
    mechanism AttributeError). It holds no pointers. With its own checks it
    takes about twice as long as the Segment (cs.v 1190 ns vs seg.v 530 ns
    measured); for tight loops see ``seg.ptr()``.

    Once ``sec.nseg`` or NEURON's topology/diameter counters
    (``api.get_topology_version()``) change after construction or the last
    ``refresh()``, every access raises ``RuntimeError`` until ``refresh()``
    is called. The values were never stale; this tells a caller that the
    segment's node or the model's setup changed under it.

    Example::

        cs = soma(0.5).cached()
        cs.v = -65.0
        soma.nseg = 11
        cs.refresh()
    """

    __slots__ = ("_seg", "_topo_ver_at_cache", "_nseg_at_cache")

    def __init__(self, segment):
        if not isinstance(segment, Segment):
            raise TypeError(f"expected Segment, got {type(segment).__name__}")
        object.__setattr__(self, "_seg", segment)
        object.__setattr__(self, "_topo_ver_at_cache", get_topology_version())
        object.__setattr__(self, "_nseg_at_cache", segment._sec.nseg)

    @property
    def x(self):
        return self._seg.x

    @property
    def sec(self):
        return self._seg.sec

    def _check_topology(self):
        # The nseg property also raises for a section deleted through HOC.
        if self._seg._sec.nseg != self._nseg_at_cache:
            raise RuntimeError(
                "CachedSegment is stale: nseg changed since cache. Call refresh()."
            )
        # get_topology_version() without a call
        if (
            _diam_changed.value,
            _structure_change_count(),
        ) != self._topo_ver_at_cache:
            raise RuntimeError(
                "CachedSegment is stale: topology or diameter changed "
                "since cache (nseg, diam, finitialize, or similar). "
                "Call refresh()."
            )

    def __getattr__(self, name):
        # Runs only when normal lookup misses (properties and slots come first).
        if name.startswith("_"):
            raise AttributeError(name)
        self._check_topology()
        return getattr(self._seg, name)

    def __setattr__(self, name, value):
        if name in CachedSegment.__slots__:
            object.__setattr__(self, name, value)
            return
        self._check_topology()
        setattr(self._seg, name, value)

    def refresh(self):
        """Accept the current nseg and topology/diameter counters."""
        object.__setattr__(self, "_topo_ver_at_cache", get_topology_version())
        object.__setattr__(self, "_nseg_at_cache", self._seg._sec.nseg)

    def __repr__(self):
        return f"<CachedSegment {self._seg!r}>"
