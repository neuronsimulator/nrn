"""Section, Segment wrappers — topology, range variables, tree traversal, psection."""
import ctypes
import sys
import weakref
from .nrnref import NrnRangeVarRef
# Hot-path imports hoisted from the bodies of __getattr__/__setattr__.
# Measurement dates below: benchmarks/README.md#historical-implementation-measurements.
# Profiling showed `from .api import ...` inside the hot
# loop was the single biggest source of per-call overhead (~50% of
# seg.v time spent in importlib._bootstrap.parent / str.rpartition).
# api.py has no back-edge to this module, so the top-level import is
# safe at module load.
from .api import (
    _diam_changed as _DIAM_CHANGED,
    _nrn_rangevar_get as _RANGEVAR_GET,
    _nrn_rangevar_set as _RANGEVAR_SET,
    _nrn_section_diam_get as _SEG_DIAM_GET,
    _nrn_section_diam_set as _SEG_DIAM_SET,
    _nrn_section_is_active as _SECTION_IS_ACTIVE,
    _nrn_symbol as _NRN_SYMBOL,
    _nrn_symbol_type as _NRN_SYMBOL_TYPE,
    TYPES as _TYPES,
    _nrn_allsec as _NRN_ALLSEC,
    _nrn_sectionlist_iterator_new as _SL_ITER_NEW,
    _nrn_sectionlist_iterator_done as _SL_ITER_DONE,
    _nrn_sectionlist_iterator_next as _SL_ITER_NEXT,
    _nrn_sectionlist_iterator_free as _SL_ITER_FREE,
)


# HOC helper procs per array-rangevar name. HOC can't dereference a range
# variable through a string arg ($s1), so we register one get/set proc per
# distinct rangevar name on first access and cache the names here.
_ARRAY_RANGEVAR_PROCS = set()


# Symbol-info cache for Segment.__getattr__/__setattr__. Maps a name (str)
# to (sym_ptr, sym_type, is_array, array_length).
#
# WHY: the encode/hash-lookup chain through nrn_symbol + nrn_symbol_type +
# nrn_symbol_is_array + nrn_symbol_array_length costs ~3.5us per call out
# of the ~4.7us seg.v total. Real NEURON uses pmech_types/rangevars_
# Python dicts for O(1) lookup at nrnpy_nrn.cpp:3146-3155.
# Populated on first lookup; HOC global symbols are stable for the process
# lifetime — once `gnabar_hh` exists, its Symbol* never moves.
# INVALIDATES: never. A (0, 0, False, 0) sentinel means the name resolves
# to no symbol (cached so misses don't repeat the lookup).
_SYMBOL_INFO_CACHE = {}
_MISSING_SYMBOL_INFO = (0, 0, False, 0)
_V_SYMBOL = _NRN_SYMBOL(b"v")
_CONNECT_ARG_UNSET = object()


def _get_symbol_info(name):
    """Return (sym_ptr, sym_type, is_array, array_length) for *name*, or None
    if no such symbol exists. Cached after first lookup."""
    cached = _SYMBOL_INFO_CACHE.get(name)
    if cached is not None:
        return cached if cached is not _MISSING_SYMBOL_INFO else None
    # These four are cold-path only (cache miss); not hoisted to module level.
    from .api import (
        _nrn_symbol,
        _nrn_symbol_type,
        _nrn_symbol_is_array,
        _nrn_symbol_array_length,
    )

    sym = _nrn_symbol(name.encode("utf-8"))
    if not sym:
        _SYMBOL_INFO_CACHE[name] = _MISSING_SYMBOL_INFO
        return None
    sym_type = int(_nrn_symbol_type(sym))
    is_array = bool(_nrn_symbol_is_array(sym))
    array_length = int(_nrn_symbol_array_length(sym)) if is_array else 0
    info = (sym, sym_type, is_array, array_length)
    _SYMBOL_INFO_CACHE[name] = info
    return info


def _install_array_rangevar_procs(name):
    if name in _ARRAY_RANGEVAR_PROCS:
        return
    from . import NEURON

    n = NEURON()
    # $1 = array index, $2 = segment position x, $3 = value (set only)
    n(f"func _mn_rv_get_{name}() {{ return {name}[$1]($2) }}")
    n(f"proc _mn_rv_set_{name}() {{ {name}[$1]($2) = $3 }}")
    _ARRAY_RANGEVAR_PROCS.add(name)


# A mechanism POINTER variable (membfunc.h NRNPOINTER). nrn_rangevar_get/set
# dereference it without a check, and the core throws through ctypes (process
# abort) when it was never set. Small HOC helpers called through the nothrow
# function API report NEURON's own "wasn't made to point to anything" instead.
_NRNPOINTER = 4
_POINTER_RANGEVARS = {}
_POINTER_RANGEVAR_PROCS = set()


def _is_pointer_rangevar(name, sym):
    flag = _POINTER_RANGEVARS.get(name)
    if flag is None:
        from .api import _nrn_symbol_subtype

        flag = _POINTER_RANGEVARS[name] = int(_nrn_symbol_subtype(sym)) == _NRNPOINTER
    return flag


def _pointer_rangevar_procs(name):
    from . import NEURON

    n = NEURON()
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
    from .object import _SuppressCStderr

    n = _pointer_rangevar_procs(name)
    try:
        # NEURON reports these as Python exceptions without printing a HOC
        # error trace; the helper's trace is suppressed to match.
        with _SuppressCStderr():
            return getattr(n, f"{proc}_{name}")(x, *args, sec=section)
    except RuntimeError as exc:
        # The core's own messages are the only signal for these two states;
        # there is no public nothrow range-variable accessor reporting them.
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
    import numbers

    if isinstance(value, bool) or not isinstance(value, numbers.Real):
        raise ValueError(f"bad value for {name}: must be a double")
    _call_pointer_proc(section, x, name, "_mn_ptr_set", float(value))


# ── _ArrayRangeVar ────────────────────────────────────────────────────────────

def _diam_node_x(sec, x):
    # The C diam accessors use node_exact (nrnoc/cabcode.cpp:1800), whereas
    # Python diameter access resolves boundary positions to the adjacent
    # interior segment — the 0/1 ends address a shared parent or terminal
    # node, not diam storage. Reads and writes must remap identically or a
    # written endpoint reads back the unrelated end-node value.
    if x == 0.0:
        return 0.5 / sec.nseg
    if x == 1.0:
        return 1.0 - 0.5 / sec.nseg
    return x


def _diam_hoc_assign(sec, cmd, x, value):
    # Public HOC assignment reaches nrn_diam_change via
    # range_interpolate_single (nrnoc/cabcode.cpp:1097), retaining the
    # core's segment-to-pt3d mapping and length adjustment. Issued for
    # every section — on a no-3d section it re-writes the same node
    # (parity-verified), and gating on sec.n3d() cost more than
    # it saved (~10 µs full HOC dispatch vs ~6 µs for this call). The raw
    # node store happens first: pt3dconst(1) suppresses the HOC assignment
    # while Python still writes the node and dirties geometry.
    from . import NEURON

    try:
        NEURON()(cmd, sec=sec, raise_on_error=True, _no_epoch_bump=True)
    except RuntimeError as exc:
        raise ValueError(
            f"diam assignment failed for {sec.name()}({x}) = {value!r}"
        ) from exc


class _ArrayRangeVar:
    """Subscriptable wrapper for an array-valued range variable on a segment.

    Used for the extracellular mechanism (xraxial/xg/xc). Access goes through
    two one-time-registered HOC helper procs because NEURON 9's C API exposes
    nrn_rangevar_get/set/push only for scalar symbols.
    """

    __slots__ = ("_section", "_x", "_name", "_size")

    def __init__(self, section, x, name, size):
        self._section = section
        self._x = x
        self._name = name
        self._size = size

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
                f"for array range variable {self._name}"
            )
        return idx

    def __getitem__(self, idx):
        from . import NEURON

        idx = self._resolve(idx)
        _install_array_rangevar_procs(self._name)
        n = NEURON()
        getter = getattr(n, f"_mn_rv_get_{self._name}")
        return float(getter(idx, self._x, sec=self._section))

    def __setitem__(self, idx, value):
        from . import NEURON

        idx = self._resolve(idx)
        _install_array_rangevar_procs(self._name)
        n = NEURON()
        setter = getattr(n, f"_mn_rv_set_{self._name}")
        setter(idx, self._x, float(value), sec=self._section)

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
        return f"<ArrayRangeVar {self._name}@{self._section.name()}({self._x}) {vals}>"


# ── Segment ───────────────────────────────────────────────────────────────────

class Segment:
    """A point at normalized position ``x`` in [0, 1] along a ``Section``.

    Obtain via ``sec(x)`` (e.g. ``soma(0.5)``). Range variables and
    mechanism properties are accessed as attributes: ``seg.v``,
    ``seg.diam``, ``seg.hh.gnabar``. A segment wrapper is a non-owning
    handle — the underlying state lives in the parent Section's node
    array, indexed by ``x``.
    """

    __slots__ = ("_sec", "_x", "_mech_cache")

    def __init__(self, sec, x):
        # _mech_cache is a lazily-populated per-Segment dict (see __getattr__).
        # With __slots__, accessing an unset slot raises AttributeError and
        # cascades into __getattr__, so initialise it up front to None.
        super().__setattr__("_sec", sec)
        super().__setattr__("_x", x)
        super().__setattr__("_mech_cache", None)

    @property
    def sec(self):
        return self._sec

    @property
    def x(self):
        return self._x

    @property
    def v(self):
        """Membrane voltage through the optimized scalar range-var path."""
        sec = self._sec
        sec._check_alive()
        return _RANGEVAR_GET(_V_SYMBOL, sec._sec, self._x)

    def __repr__(self):
        return f"{self._sec.name()}({self.x})"

    @staticmethod
    def _node_key(x, nseg):
        # Real NEURON's node model: x=0 and x=1 are their own boundary nodes
        # (distinct from the interior), while every interior x within a segment
        # shares that segment's center node. Plain int(x*nseg) binning would
        # merge the x=0 boundary into the first interior segment, making
        # s(0) == s(0.1). Compute the key without an FFI call.
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
        # Consistent with __eq__ (same node key). Real NEURON's own hash is
        # x-based and inconsistent with its node-based eq (two equal interior
        # segments can hash differently); myneuron keeps the Python-correct
        # consistent hash. Deliberate departure — see docs/ARCHITECTURE.md.
        nseg = self._sec.nseg
        return hash((self._sec, self._node_key(self._x, nseg)))

    def ref(self, name):
        return NrnRangeVarRef(self.sec, self.x, name)

    def _check_rangevar_mechanism(self, name):
        """Raise AttributeError if a suffixed range variable's mechanism
        is not inserted in this section."""
        # Range variables like gnabar_hh have a mechanism suffix.
        # Unsuffixed vars (v, diam, cm, ena, ek, ...) are always valid.
        idx = name.rfind("_")
        if idx <= 0:
            return
        suffix = name[idx + 1:]
        mech_sym = _NRN_SYMBOL(suffix.encode("utf-8"))
        if mech_sym and int(_NRN_SYMBOL_TYPE(mech_sym)) == _TYPES.MECHANISM:
            from . import NEURON

            if not NEURON().ismembrane(suffix, sec=self._sec):
                raise AttributeError(
                    f"mechanism '{suffix}' is not inserted in {self._sec!r}"
                )

    def area(self):
        """Return membrane area of this segment in um^2."""
        from . import NEURON
        return float(NEURON().area(self._x, sec=self._sec))

    def ri(self):
        """Return axial resistance to parent segment in MOhm."""
        from . import NEURON
        return float(NEURON().ri(self._x, sec=self._sec))

    def volume(self):
        """Return volume of this segment in um^3.

        Approximates each segment as a cylinder (pi * r^2 * L/nseg); accurate
        only when diam is uniform within the segment.
        """
        import math
        r = self.diam / 2.0
        seg_length = self._sec.L / self._sec.nseg
        return math.pi * r * r * seg_length

    def point_processes(self):
        """Return list of point processes attached to this segment.

        Iterates all object classes that have a ``has_loc`` method
        (point processes), checks each instance's location against
        this segment.
        """
        from . import NEURON
        from .api import _nrn_section_pop, _nrn_cas
        from .utils import list_functions, list_methods

        n_inst = NEURON()
        result = []

        # Phase 1: collect PP class names from symbol table.
        # Find type-325 classes that advertise has_loc. Skip PythonObject —
        # calling nrn_symbol_table on it segfaults because its symbol table
        # holds Python interpreter state, not HOC symbols, and the C iterator
        # dereferences it as a Symlist.
        _SKIP_CLASSES = {"PythonObject"}
        all_syms = list_functions()
        from .api import TYPES
        pp_classes = []
        for name, (t, _) in all_syms.items():
            if t != TYPES.TEMPLATE or name in _SKIP_CLASSES:
                continue
            # Some type-325 entries (HOC templates with no methods, deprecated
            # classes) raise on list_methods. Swallow and skip — we just want
            # the ones that genuinely advertise has_loc.
            try:
                methods = list_methods(name)
                if "has_loc" in methods:
                    pp_classes.append(name)
            except Exception:
                pass

        nseg = self._sec.nseg

        def _node_key(x):
            # Membership = same NEURON node (node_exact), NOT int(x*nseg) bins.
            # x=0 and x=1 are the zero-area boundary nodes, each distinct from
            # every interior segment; only interior x falls in segment
            # int(x*nseg). Plain int(x*nseg) would lump an x=0 point process
            # into segment 0 (and any bin-boundary x into a neighbor).
            if x <= 0.0:
                return -1            # 0-end boundary node
            if x >= 1.0:
                return nseg          # 1-end boundary node
            b = int(x * nseg)
            return b if b < nseg else nseg - 1

        my_key = _node_key(self._x)

        # Phase 2: iterate instances of each PP class.
        for cls_name in pp_classes:
            try:
                lst = n_inst.List(cls_name)
            except Exception:
                continue
            count = int(lst.count())
            if count == 0:
                continue
            for j in range(count):
                pp = lst.object(j)
                try:
                    if int(pp.has_loc()) != 1:
                        continue
                    # Phase 3: bin-compare location to this segment.
                    loc = pp.get_loc()
                    pp_sec_ptr = _nrn_cas()
                    _nrn_section_pop()  # get_loc pushes section
                    if pp_sec_ptr == self._sec._sec and _node_key(loc) == my_key:
                        result.append(pp)
                except Exception:
                    continue
        return result

    def node_index(self):
        """Return the internal NEURON node index for this segment.

        Mirrors real NEURON's ``segment.node_index()``. Indices are canonical
        once the tree is set up (after ``finitialize()``).

        Uses ``nrn_segment_node_index`` (gap-15, merged upstream as nrn#3810).
        """
        from .api import _nrn_segment_node_index

        self._sec._check_alive()
        return int(_nrn_segment_node_index(self._sec._sec, self._x))

    def __dir__(self):
        attrs = set(super().__dir__())
        # Standard segment attributes always available
        attrs.update(["v", "diam", "cm", "x", "sec", "area", "ri", "volume",
                       "point_processes", "node_index", "ref"])
        # Add _ref_ variants for standard range vars
        attrs.update(["_ref_v", "_ref_diam", "_ref_cm"])
        # Add mechanism names inserted in this section
        from .mechanism import _all_density_mechanism_names
        from . import NEURON
        try:
            n_inst = NEURON()
            for mech_name in _all_density_mechanism_names():
                if n_inst.ismembrane(mech_name, sec=self._sec):
                    attrs.add(mech_name)
            # Also add ion mechanisms
            for ion_name in ("na_ion", "k_ion", "ca_ion"):
                from .api import _nrn_symbol
                sym = _nrn_symbol(ion_name.encode("utf-8"))
                if sym and n_inst.ismembrane(ion_name, sec=self._sec):
                    attrs.add(ion_name)
        except Exception:
            pass
        return sorted(attrs)

    def __getattr__(self, name):
        if name.startswith("_ref_"):
            name = name[5:]
            info = _get_symbol_info(name)
            if info is None or info[1] != _TYPES.RANGEVAR:
                raise AttributeError(f"Variable '{name}' not found in {self!r}.")
            if _is_pointer_rangevar(name, info[0]):
                _pointer_rangevar_get(self.sec, self.x, name)  # raises if unset
            return NrnRangeVarRef(self.sec, self.x, name)

        # Mechanism instance cache (Segment._mech_cache).
        # WHY: rebuilding a Mechanism wrapper on every seg.hh access
        # dominated seg.hh.gnabar cost (__init__ + __getattr__ symbol walk).
        # INVALIDATES: never per Segment. The Mechanism wrapper is
        # stateless beyond (seg, name) — different segments get their own
        # cache because each Segment is a distinct Python object.
        # Stored in a dedicated __slots__ entry; reads go straight to the
        # slot descriptor (no __dict__ allocation per Segment).
        mech_cache = self._mech_cache
        if mech_cache is not None:
            cached_mech = mech_cache.get(name)
            if cached_mech is not None:
                return cached_mech

        info = _SYMBOL_INFO_CACHE.get(name)
        if info is None:
            info = _get_symbol_info(name)
        elif info is _MISSING_SYMBOL_INFO:
            info = None
        if info is None:
            raise AttributeError(f"Variable '{name}' not found in {self!r}.")
        sym, sym_type, is_array, array_length = info
        if sym_type == _TYPES.RANGEVAR:
            # Liveness guard before touching the C node: reading a range var on
            # a HOC-deleted section dereferences a freed node and SIGABRTs
            # (node_index assertion, cabcode.cpp). Real NEURON raises here, and
            # so do we. Scoped to the range-var path — the
            # mechanism-cache and seg.ptr()/cached() hot loops bypass it, and
            # real NEURON pays an equivalent check on this same access.
            sec = self._sec
            sec._check_alive()
            # Inline the cheap unsuffixed-name fast path. Mechanism
            # suffixes only matter for names like `gnabar_hh`; for `v`,
            # `diam`, `cm`, etc. there's nothing to check. Skipping the
            # function call saves ~0.3 µs/access.
            if name.rfind("_") > 0:
                self._check_rangevar_mechanism(name)
            if name == "diam":
                # Route diam through nrn_segment_diam_get so a
                # 3d-defined section reports the recomputed value rather than a
                # stale default before a geometry pass. The generic rangevar
                # read hits the raw pointer and skips NEURON's per-section
                # recalc_area_ recompute; the getter performs it (on a libnrniv
                # carrying the upstream fix — older builds return the same value
                # the generic read would, so this is safe either way).
                # A non-positive diameter on a stylized section is NEURON's
                # deferred error: the recompute clamps exactly ONE offending
                # node per read (the last) and raises the literal
                # "diam = 0." message, again on every read until none remain.
                # On the supported dev114 core, nrn#3854 makes the public C
                # getter suppress hoc_execerror at the ABI. Keep the scan to
                # reproduce the error real Python raises, not for C-ABI
                # safety (preventing an escaping throw was its historical
                # role before nrn#3854). Gated on NEURON's own dirty flag
                # (cleared by finitialize/global recompute, so hot loops pay
                # one FFI call: 0.77 µs vs 58 µs ungated at nseg=101;
                # while a recompute is pending the scan is
                # O(nseg), the price of raising where real raises). The
                # n3d guard scopes the scan to
                # nrn_area_ri's no-3d-points branch — diam_from_list never
                # raises, and a 3d section's raw node value is not what the
                # getter returns anyway.
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
            if is_array:
                return _ArrayRangeVar(sec, self._x, name, array_length)
            if _is_pointer_rangevar(name, sym):
                return _pointer_rangevar_get(sec, self._x, name)
            return _RANGEVAR_GET(sym, sec._sec, self._x)
        if sym_type == _TYPES.MECHANISM:
            from .mechanism import Mechanism

            # Only an inserted mechanism is accessible as seg.<mech>. Real NEURON
            # raises AttributeError for a known-but-not-inserted mechanism;
            # returning a Mechanism wrapper anyway lets a later range-var read
            # dereference an absent prop and SIGABRT. Cold path
            # only — warm hits return the cached wrapper above. Check the
            # per-section inserted-mech cache first (built once per section,
            # amortized across its segments, invalidated by Python insert/
            # uninsert); has_membrane (one ismembrane call) is the fallback for
            # ions, which the density-only cache omits, and the absent case.
            self._sec._check_alive()
            if name not in self._sec._get_inserted_mechs() and not self._sec.has_membrane(name):
                raise AttributeError(
                    f"'{name}' is not inserted in {self._sec!r}."
                )
            mech = Mechanism(self, name)
            if mech_cache is None:
                mech_cache = {}
                object.__setattr__(self, "_mech_cache", mech_cache)
            mech_cache[name] = mech
            return mech
        if _TYPES.RANGEOBJ is None or sym_type == _TYPES.RANGEOBJ:
            from .utils import _try_wrap_density_nmodlrandom

            self._sec._check_alive()
            wrapped = _try_wrap_density_nmodlrandom(
                self._sec._sec, self._x, sym, sym_type
            )
            if wrapped is not None:
                return wrapped
        raise AttributeError(
            f"'{name}' is not a range variable on {self!r}."
        )

    def __iter__(self):
        """Iterate over Mechanism objects for each inserted mechanism."""
        from .mechanism import Mechanism

        mech_cache = self._mech_cache
        for mech_name in self._sec._get_inserted_mechs():
            if mech_cache is not None and mech_name in mech_cache:
                yield mech_cache[mech_name]
                continue
            mech = Mechanism(self, mech_name)
            if mech_cache is None:
                mech_cache = {}
                object.__setattr__(self, "_mech_cache", mech_cache)
            mech_cache[mech_name] = mech
            yield mech

    def __setattr__(self, name, value):
        # Internal bookkeeping (all underscore-prefixed slots) goes straight
        # through to the slot descriptor — these are never range vars.
        if name.startswith("_"):
            super().__setattr__(name, value)
            return
        info = _get_symbol_info(name)
        if info is None:
            raise AttributeError(f"Variable '{name}' not found in {self!r}.")
        sym, sym_type, is_array, array_length = info
        from .api import TYPES
        if TYPES.RANGEOBJ is None or sym_type == TYPES.RANGEOBJ:
            from .utils import _try_wrap_density_nmodlrandom

            self._sec._check_alive()
            wrapped = _try_wrap_density_nmodlrandom(
                self._sec._sec, self._x, sym, sym_type
            )
            if wrapped is not None:
                raise ValueError(
                    f"NMODL RANDOM variable '{name}' is not assignable"
                )
        if sym_type != TYPES.RANGEVAR:
            raise AttributeError(
                f"'{name}' is not a range variable on {self!r}."
            )
        # Guard before writing a potentially freed C node; see __getattr__.
        self._sec._check_alive()
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
            # reports them on the next diam/geometry read. Preserve that
            # behavior, while still rejecting NaN/Inf at the Python boundary.
            import math
            if not math.isfinite(value):
                raise ValueError(
                    f"diam must be a finite number, got {value!r}"
                )
            sec = self._sec
            _SEG_DIAM_SET(sec._sec, _diam_node_x(sec, self._x), value)
            _diam_hoc_assign(
                sec, f"diam({self._x!r}) = {float(value)!r}", self._x, value
            )
            return
        if _is_pointer_rangevar(name, sym):
            _pointer_rangevar_set(self._sec, self._x, name, value)
            return
        _RANGEVAR_SET(sym, self._sec._sec, self._x, value)

    def cached(self):
        """Return a CachedSegment for this segment.

        Note: attribute-access overhead means cs.v is not faster than seg.v.
        For tight loops, use seg.ptr('v')[0] directly (~15x faster than seg.v).
        """
        return CachedSegment(self)

    def ptr(self, name):
        """Return the raw ``c_double *`` for a scalar range variable.

        Reads and writes via the returned pointer (``p[0]``,
        ``p[0] = value``) bypass ``__getattr__``, symbol lookup, ctypes
        argument marshaling, and the ``nrn_rangevar_get`` FFI call. In
        a tight loop this is roughly 15x faster than ``seg.<name>``
        (and faster than real NEURON's CPython extension, which still
        resolves the symbol on every access).

        The pointer is valid until any of:
          - the section is deleted
          - ``sec.nseg`` is changed (from Python OR from HOC)
          - the model is topology-mutated in a way that reallocates nodes
          - ``finitialize`` runs after a structural change

        No staleness check is performed: reading through a stale pointer
        returns garbage, writing through one silently corrupts memory.
        Re-acquire by calling ``seg.ptr(name)`` again, or use
        ``seg.cached()`` which raises on stale access via the
        ``diam_changed`` / ``structure_change_cnt`` C counters.

        Only scalar range variables (HOC symbol type 310) are
        addressable. Array range vars and ion globals raise
        ``AttributeError``.
        """
        info = _get_symbol_info(name)
        if info is None:
            raise AttributeError(f"unknown range variable {name!r}")
        sym, sym_type, is_array, _ = info
        from .api import TYPES, _nrn_rangevar_push, _nrn_double_ptr_pop
        if sym_type != TYPES.RANGEVAR or is_array:
            raise AttributeError(
                f"{name!r} is not a scalar range variable "
                f"(type {sym_type}, array={is_array})"
            )
        # Acquiring the pointer pushes a rangevar address for the C node; on a
        # HOC-deleted section or for a mechanism not inserted at this segment
        # that dereferences freed/absent state and SIGABRTs. Guard
        # both, matching real NEURON (ReferenceError / AttributeError). This is
        # the acquisition, not the returned pointer — the pointer's staleness
        # contract (docstring above) is unchanged.
        self._sec._check_alive()
        self._check_rangevar_mechanism(name)
        _nrn_rangevar_push(sym, self._sec._sec, ctypes.c_double(self._x))
        raw = _nrn_double_ptr_pop()
        if not raw:
            raise RuntimeError(f"could not acquire pointer for {name!r}")
        return ctypes.cast(raw, ctypes.POINTER(ctypes.c_double))


# ── Section ───────────────────────────────────────────────────────────────────

# Counter for auto-generated names of a nameless Section() — mirrors real
# NEURON, whose h.Section() with no name auto-generates __nrnsec_0x<address>.
_anon_section_count = 0


class Section:
    """A NEURON section — a continuous unbranched cable, the unit of topology.

    Create with ``n.Section('soma')`` (canonical) or ``Section('soma')``.
    Connect with ``dend.connect(soma(1), 0)``. Set section-level
    properties via attributes (``soma.L = 20``, ``soma.diam = 20``,
    ``soma.nseg = 5``, ``soma.Ra = 100``). Insert density mechanisms
    with ``soma.insert('hh')``. Iterate segments by index or via
    ``for seg in soma: ...``; access a specific segment with
    ``soma(x)`` for ``x`` in [0, 1].
    """

    # __weakref__ lets Python-owned Sections be weakly held by dependents, so
    # their last owning wrapper still controls native lifetime. Non-owning
    # HOC/template wrappers are retained strongly by dependents (gap-50).
    __slots__ = (
        "_cell",
        "_sec",
        "_pynative",
        "_inserted_mechs",
        "_inserted_mechs_epoch",
        "__weakref__",
    )

    def __init__(self, name=None, cell=None, _pynative=True):
        from .api import _nrn_section_new

        if cell is not None and isinstance(cell, str):
            raise TypeError("cell must be an object, not a string")
        if name is None:
            # h.Section() with no name: auto-generate a unique one, as real
            # NEURON does (it names anonymous sections __nrnsec_0x<address>).
            global _anon_section_count
            name = "__nrnsec_0x%x" % _anon_section_count
            _anon_section_count += 1
        self._cell = weakref.ref(cell) if cell is not None else None
        if "\x00" in name:
            # ctypes' c_char_p accepts bytes-with-NUL without firing its guard,
            # and nrn_section_new then truncates at the first NUL — two names
            # differing only after a NUL would collapse to the same section.
            # Reject like real NEURON (ValueError) instead.
            raise ValueError("embedded null character")
        self._sec = _nrn_section_new(name.encode("utf-8"))
        self._pynative = _pynative
        # Lazily populated by _get_inserted_mechs; invalidated on
        # insert/uninsert/nseg-change by setting back to None, and on any
        # HOC-side mutation via the model epoch.
        self._inserted_mechs = None
        self._inserted_mechs_epoch = -1

    @classmethod
    def _from_ptr(cls, sec_ptr):
        """Create a non-owning Section wrapper from a raw C Section* pointer.

        Hot path: only the two slots that change between non-owning
        wrappers are written (`_sec` and `_pynative`). The lazy slots
        `_cell` and `_inserted_mechs` are left unset; their readers
        (`Section.cell()` and `Section._get_inserted_mechs()`) treat
        an unset slot as the conceptual "None" default. That saves two
        `object.__setattr__` calls per yielded section — about a third
        of `_from_ptr`'s cost — and is on the dominant cost line in
        `allsec()`.

        CAUTION: any caller that adds code to _from_ptr paths must use
        object.__getattribute__ for _cell and _inserted_mechs; normal
        attribute access recurses via __getattr__.
        """
        obj = cls.__new__(cls)
        _object_setattr = object.__setattr__
        _object_setattr(obj, "_sec", sec_ptr)
        _object_setattr(obj, "_pynative", False)
        return obj

    def _check_alive(self):
        """Raise ``RuntimeError`` if this section has been deleted.

        Project rule: ctypes-based liveness checks raise ``RuntimeError``
        (NEURON's section table no longer recognises the pointer);
        weakref-based checks (nrnref.py, Object._check_host_alive) raise
        ``ReferenceError``.
        """
        if not _SECTION_IS_ACTIVE(self._sec):
            raise RuntimeError("accessing a deleted section")

    def __eq__(self, other):
        if not isinstance(other, Section):
            return NotImplemented
        return self._sec == other._sec

    def __hash__(self):
        return hash(self._sec)

    def __setattr__(self, name, value):
        # Private names and known @property setters go to super();
        # everything else broadcasts across segments — matches real NEURON's
        # `sec.gnabar_hh = ...` semantics.
        if name.startswith("_") or name in {"nseg", "L", "Ra", "rallbranch"}:
            super().__setattr__(name, value)
        elif name == "diam":
            # One HOC-level broadcast reaches nrn_diam_change once instead
            # of nseg round-trips (2.25 ms -> single-call cost at nseg=101).
            # Raw per-node stores first, mirroring
            # the per-segment path: under pt3dconst(1) they are the only
            # write that lands.
            import math
            if not math.isfinite(value):
                raise ValueError(
                    f"diam must be a finite number, got {value!r}"
                )
            nseg = self.nseg
            for i in range(nseg):
                _SEG_DIAM_SET(self._sec, (i + 0.5) / nseg, value)
            _diam_hoc_assign(self, f"diam = {float(value)!r}", "0:1", value)
        else:
            for seg in self:
                setattr(seg, name, value)

    def __dir__(self):
        attrs = set(object.__dir__(self))
        attrs.update([
            "L", "Ra", "nseg", "rallbranch",
            "allseg", "arc3d", "cell", "children", "connect",
            "diam3d", "disconnect", "has_membrane", "hname",
            "hoc_internal_name", "insert", "is_pysec", "n3d", "name",
            "orientation", "parentseg", "psection", "pt3dadd",
            "pt3dchange", "pt3dclear", "pt3dinsert", "pt3dremove",
            "pt3dstyle", "push", "rallbranch", "same", "spine3d",
            "subtree", "trueparentseg", "uninsert", "wholetree",
            "x3d", "y3d", "z3d",
        ])
        return sorted(attrs)

    def __getattr__(self, name):
        # An unset _sec slot (partial __init__) would route through here
        # again via _check_alive, recursing. Surface AttributeError
        # directly in that case.
        try:
            object.__getattribute__(self, "_sec")
        except AttributeError:
            raise AttributeError(name) from None
        self._check_alive()
        return getattr(self(0.5), name)

    def __iter__(self):
        self._check_alive()
        # Real NEURON re-evaluates nseg after each yield, so rediscretizing a
        # section changes both the remaining midpoints and the stopping point.
        i = 0
        while i < self.nseg:
            yield self((i + 0.5) / self.nseg)
            i += 1

    def allseg(self):
        """Iterate all segments including endpoints at x=0 and x=1."""
        yield self(0)
        yield from self
        yield self(1)

    def insert(self, mechanism):
        from .api import _nrn_mechanism_insert, _nrn_symbol
        from .mechanism import DensityMechanism

        # Direct-C path (no sec= push): guard explicitly. Inserting into a
        # section freed through HOC SIGSEGVs the core.
        self._check_alive()
        if isinstance(mechanism, DensityMechanism):
            mechanism = mechanism.name
        if not isinstance(mechanism, bytes):
            mechanism_bytes = mechanism.encode("utf-8")
        else:
            mechanism_bytes = mechanism
            mechanism = mechanism_bytes.decode("utf-8")
        mech_symbol = _nrn_symbol(mechanism_bytes)

        if not mech_symbol:
            # Enumerate insertable mechanisms from the cached top-level
            # symbol table so the user can spot a typo.
            from . import NEURON
            from .api import TYPES
            available = sorted(
                name for name, (sym_type, _) in NEURON()._top_level.items()
                if sym_type == TYPES.MECHANISM
            )
            hint = ", ".join(available) if available else "(none — run nrnivmodl)"
            raise ValueError(
                f"Mechanism {mechanism!r} is not defined. "
                f"Available mechanisms: {hint}"
            )
        _nrn_mechanism_insert(self._sec, mech_symbol)
        from . import _bump_model_epoch

        _bump_model_epoch()
        self._inserted_mechs = None

    def uninsert(self, mechanism):
        """Remove a previously inserted mechanism.

        Note: ``insert()`` calls the C API directly (``nrn_mechanism_insert``),
        but ``uninsert()`` routes through the HOC ``uninsert`` statement
        because there is no symmetric ``nrn_mechanism_uninsert`` in
        ``neuronapi.h`` (gap-8, CLOSED via this HOC fallback). The HOC
        path is correct — just slightly higher-overhead than insert.
        """
        from .mechanism import DensityMechanism

        if isinstance(mechanism, DensityMechanism):
            mechanism = mechanism.name
        if not self.has_membrane(mechanism):
            raise ValueError(f"mechanism '{mechanism}' is not inserted")
        from . import NEURON

        NEURON()(f"uninsert {mechanism}", sec=self)
        self._inserted_mechs = None

    def _get_inserted_mechs(self):
        """Return the cached list of mechanism names inserted on this section.

        WHY: Segment.__iter__ used to call ismembrane(name) for every
        density mechanism on every iteration — an N×M HOC roundtrip
        where M = total registered mechs.
        INVALIDATES: local insert()/uninsert()/nseg-set by setting the slot
        back to None, plus cross-wrapper/HOC mutations through the global
        model epoch. Kept on the Section instance (not class-level)
        because the inserted-mech set is per-section state, but multiple
        Python wrappers can still alias the same native Section*.

        The slot may also be entirely unset on non-owning wrappers
        produced by `_from_ptr` — those skip the lazy-slot init in the
        allsec() hot path. Use object.__getattribute__ to bypass the
        __getattr__/__repr__ cycle described in Section.cell(); treat
        an unset slot as the conceptual None.
        """
        try:
            cached = object.__getattribute__(self, "_inserted_mechs")
        except AttributeError:
            cached = None
        from . import NEURON, _model_epoch

        epoch = _model_epoch()
        if cached is not None:
            # A HOC-side insert/uninsert (n("insert pas")) does not run the
            # local Python invalidation, and a direct insert through a different
            # wrapper only clears that wrapper's slot. Also require the cache to
            # be from the current model epoch.
            # The epoch slot may be unset on _from_ptr wrappers — treat that as
            # stale.
            try:
                built = object.__getattribute__(self, "_inserted_mechs_epoch")
            except AttributeError:
                built = -1
            if built == epoch:
                return cached
        from .mechanism import _all_density_mechanism_names

        n = NEURON()
        inserted = [
            name
            for name in _all_density_mechanism_names()
            if n.ismembrane(name, sec=self)
        ]
        self._inserted_mechs = inserted
        self._inserted_mechs_epoch = epoch
        return inserted

    def has_membrane(self, mechanism):
        """Return True if *mechanism* is inserted in this section."""
        from . import NEURON

        return bool(NEURON().ismembrane(mechanism, sec=self))

    def name(self):
        self._check_alive()
        from . import NEURON

        raw = NEURON().secname(sec=self)
        if raw.startswith("_pysec."):
            raw = raw[7:]
        cell = self.cell()
        if cell is not None:
            return f"{cell}.{raw}"
        return raw

    def __repr__(self):
        # name() is already cell-qualified (returns "<cell>.<raw>" when owned),
        # so prefixing the cell again would produce "<cell>.<cell>.<raw>".
        # Real NEURON prints the single cell-qualified name.
        return self.name()

    def __call__(self, x):
        self._check_alive()
        if 0 <= x <= 1:
            return Segment(self, x)
        else:
            raise ValueError("segment position range is 0 <= x <= 1")

    def ref(self, name, x=0.5):
        if not (0 <= x <= 1):
            raise ValueError("segment position range is 0 <= x <= 1")
        return NrnRangeVarRef(self, x, name)

    @property
    def nseg(self):
        self._check_alive()
        from .api import _nrn_nseg_get

        return _nrn_nseg_get(self._sec)

    @nseg.setter
    def nseg(self, value):
        self._check_alive()
        # NEURON's hard limit is 32767 (NSEG_MAX in cabcode.cpp). The C
        # API prints a warning and silently coerces to 1 on overflow,
        # which is a footgun — match real NEURON's Python wrapper and
        # raise instead.
        # numbers.Integral, not int: numpy.int64/int32/uint* are NOT int
        # subclasses but are everywhere in model-building loops, and real
        # NEURON accepts them. bool is Integral too (True -> nseg 1), matching
        # real NEURON.
        import numbers

        if not isinstance(value, numbers.Integral) or value <= 0 or value > 32767:
            raise ValueError(
                f"nseg must be an integer in range 1 to 32767, got {value!r}"
            )
        from .api import _nrn_nseg_set

        _nrn_nseg_set(self._sec, int(value))
        # Staleness is tracked via NEURON's own diam_changed /
        # structure_change_cnt globals (see api.get_topology_version) —
        # nrn_nseg_set already flips diam_changed, so CachedSegment
        # instances will detect this without a Python-side counter.
        self._inserted_mechs = None

    def cell(self):
        """Return the cell object owning this section, or None.

        Uses object.__getattribute__ to avoid the __getattr__ recursion path
        that fires for unset slots on non-owning wrappers produced by _from_ptr.
        """
        # Non-owning wrappers from _from_ptr leave _cell unset to save
        # a slot write on the allsec() hot path. An unset slot is the
        # conceptual "no cell binding," identical to the explicit None
        # written by the owning __init__.
        try:
            cell_ref = object.__getattribute__(self, "_cell")
        except AttributeError:
            return None
        if cell_ref is None:
            return None
        return cell_ref()

    @property
    def L(self):
        self._check_alive()
        from .api import _nrn_section_length_get

        return _nrn_section_length_get(self._sec)

    @L.setter
    def L(self, value):
        self._check_alive()
        from .api import _nrn_section_length_set

        # `value <= 0` lets NaN through (any comparison with NaN is False),
        # and `inf` would also pass a naive `> 0` check. Reject both
        # explicitly to match real NEURON's wrapper.
        import math
        if not math.isfinite(value) or value <= 0:
            raise ValueError("L must be > 0.")
        _nrn_section_length_set(self._sec, value)

    @property
    def Ra(self):
        self._check_alive()
        from .api import _nrn_section_Ra_get

        return _nrn_section_Ra_get(self._sec)

    @Ra.setter
    def Ra(self, value):
        self._check_alive()
        # The C API at neuronapi.cpp:115 has a pending "ensure val > 0"
        # note and accepts anything. Real NEURON's Python wrapper
        # validates at nrnpy_nrn.cpp:2065-2071. Match that contract here.
        # NaN slips past `value <= 0` (NaN comparisons are all False) and
        # `inf` passes `> 0` — reject both explicitly.
        import math
        if not math.isfinite(value) or value <= 0:
            raise ValueError("Ra must be > 0.")
        from .api import _nrn_section_Ra_set

        _nrn_section_Ra_set(self._sec, value)

    @property
    def rallbranch(self):
        self._check_alive()
        from .api import _nrn_section_rallbranch_get

        return _nrn_section_rallbranch_get(self._sec)

    @rallbranch.setter
    def rallbranch(self, value):
        self._check_alive()
        from .api import _nrn_section_rallbranch_set

        _nrn_section_rallbranch_set(self._sec, value)

    def push(self):
        """Push this section onto the section stack."""
        self._check_alive()
        from .api import _nrn_section_push

        _nrn_section_push(self._sec)

    def same(self, other):
        """Return True if *other* refers to the same underlying section."""
        if not isinstance(other, Section):
            return False
        return self._sec == other._sec

    def hoc_internal_name(self):
        """Return the internal HOC name (e.g. '__nrnsec_0x...')."""
        self._check_alive()
        import ctypes

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
            # reinterpreted as child_x (nrnpy_nrn.cpp::NPySecObj_connect).
            if parent_x is not _CONNECT_ARG_UNSET:
                child_x = parent_x
            parent_x = parent.x
            parent = parent.sec
        elif parent_x is _CONNECT_ARG_UNSET:
            parent_x = 1
        if not isinstance(parent, Section):
            raise TypeError(f"parent must be a Section, got {type(parent).__name__}")
        # Direct-C path: a deleted endpoint lets a C++ exception cross ctypes and
        # abort. Guard both sections explicitly.
        self._check_alive()
        parent._check_alive()
        if self._sec == parent._sec:
            raise ValueError("cannot connect a section to itself")
        # Validate before the C call. nrn_section_connect raises an
        # uncaught C++ exception on out-of-range positions, which
        # std::terminate's the process rather than surfacing through ctypes.
        if not (0.0 <= float(parent_x) <= 1.0):
            raise ValueError(f"parent_x must be in [0, 1], got {parent_x}")
        if float(child_x) not in (0.0, 1.0):
            raise ValueError(f"child connection end must be 0 or 1, got {child_x}")
        from .api import _nrn_section_connect

        _nrn_section_connect(self._sec, child_x, parent._sec, parent_x)
        return self

    def parentseg(self):
        """Return the parent Segment, or None if this is a root section."""
        from . import NEURON
        from .api import _nrn_section_parent

        n = NEURON()
        parent_ptr = _nrn_section_parent(self._sec)
        if not parent_ptr:
            return None
        parent_sec = Section._from_ptr(parent_ptr)
        parent_x = n.parent_connection(sec=self)
        return Segment(parent_sec, parent_x)

    def children(self):
        """Return a list of child Sections.

        Walks the section's own child/sibling chain via the nrn#3835 accessors,
        in the same order as real NEURON's ``SectionRef.child[i]``.
        """
        from .api import _nrn_section_child, _nrn_section_sibling

        result = []
        child_ptr = _nrn_section_child(self._sec)
        while child_ptr:
            result.append(Section._from_ptr(child_ptr))
            child_ptr = _nrn_section_sibling(child_ptr)
        return result

    def orientation(self):
        """Return which end (0 or 1) of this section is connected to parent."""
        from . import NEURON

        return NEURON().section_orientation(sec=self)

    def subtree(self):
        """Return a list of this section and all its descendants."""
        result = [self]
        for child in self.children():
            result.extend(child.subtree())
        return result

    def wholetree(self):
        """Return all sections in the tree containing this section."""
        root = self
        while True:
            ps = root.parentseg()
            if ps is None:
                break
            root = ps.sec
        return root.subtree()

    def hname(self):
        """Return the HOC section name.

        In myneuron this delegates to name() — both call n.secname(). Real
        NEURON distinguishes the two for cell-owned sections (hname() returns
        the qualified HOC path); this implementation does not.
        """
        return self.name()

    def trueparentseg(self):
        """Return the true parent Segment, or None.

        The "true parent" is the nearest ancestor that actually drives current
        into this section through the cable equation. A section connected at its
        parent's oriented beginning (``nrn_at_beginning``: the child's
        connection position equals the parent's orientation) sits at the same
        node as that parent, so it is skipped and the walk continues UP the
        ancestor chain until a section is NOT at its parent's beginning — that
        parent is the true parent. This is an ancestor walk, not a single hop:
        mirrors NEURON's ``nrn_trueparent`` / ``pysec_trueparentseg``
        (cabcode.cpp:1577, nrnpy_nrn.cpp:1096). The earlier one-hop version
        diverged for multi-level 0-connected chains.
        """
        from . import NEURON

        n = NEURON()
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
        from . import NEURON
        return int(NEURON().spine3d(i, sec=self))

    def disconnect(self):
        """Disconnect this section from its parent."""
        from . import NEURON

        NEURON()("disconnect()", sec=self)

    def __del__(self):
        # Avoid errors during Python shutdown
        if (
            getattr(sys, "meta_path", None) is None
            or getattr(sys, "modules", None) is None
        ):
            return
        # __init__ may have raised before setting _pynative / _sec (e.g. a
        # name-validation error in _nrn_section_new). With __slots__,
        # reading an unset slot routes through __getattr__, which calls
        # _check_alive() and recurses — bypass __getattr__ by going
        # straight to the slot descriptor.
        try:
            pynative = object.__getattribute__(self, "_pynative")
            sec_ptr = object.__getattribute__(self, "_sec")
        except AttributeError:
            return
        if pynative:
            from .api import _nrn_section_push, _nrn_section_is_active

            if not _nrn_section_is_active(sec_ptr):
                return

            from . import n

            # Pop via HOC `pop_section()` rather than the C
            # `nrn_section_pop`. The C API calls chk_access first, which
            # errors with "Accessing a deleted section" because delete_section
            # already invalidated the top of the stack. HOC's pop_section
            # bypasses that check (cabcode.cpp:2165) — without it, every
            # __del__ leaks one section-stack slot and the stack overflows
            # at N~200 (NSECSTACK in cabcode.cpp:73). Regression coverage and
            # closure history are in KNOWN_GAPS.md's stack-leak section.
            _nrn_section_push(sec_ptr)
            n.delete_section()
            n.pop_section()

    def psection(self):
        """Return a dict of section properties matching real NEURON's format."""
        self._check_alive()
        from . import NEURON

        n = NEURON()
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
                        short = vname[: -len(suffix)] if vname.endswith(suffix) else vname
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
            for pp in seg.point_processes():
                class_name = type(pp).__name__
                if class_name not in pp_dict:
                    pp_dict[class_name] = []
                pp_dict[class_name].append(pp)

        # secname (used for hoc_internal_name in return dict)
        raw = n.secname(sec=self)

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
            "name": self.name(),
            "hoc_internal_name": raw,
            "cell": self.cell(),
        }

    def arc3d(self, i):
        from . import NEURON

        return NEURON().arc3d(i, sec=self)

    def diam3d(self, i):
        from . import NEURON

        return NEURON().diam3d(i, sec=self)

    def pt3dadd(self, x, y, z, d):
        from . import NEURON

        return NEURON().pt3dadd(x, y, z, d, sec=self)

    def pt3dchange(self, i, *args):
        from . import NEURON

        return NEURON().pt3dchange(i, *args, sec=self)

    def pt3dclear(self, *args):
        from . import NEURON

        return NEURON().pt3dclear(*args, sec=self)

    def pt3dinsert(self, i, x, y, z, d):
        from . import NEURON

        return NEURON().pt3dinsert(i, x, y, z, d, sec=self)

    def pt3dremove(self, i):
        """remove the ith point"""
        from . import NEURON

        return NEURON().pt3dremove(i, sec=self)

    def pt3dstyle(self, *args):
        from . import NEURON

        return NEURON().pt3dstyle(*args, sec=self)

    def n3d(self):
        from . import NEURON

        return NEURON().n3d(sec=self)

    def x3d(self, i):
        from . import NEURON

        return NEURON().x3d(i, sec=self)

    def y3d(self, i):
        from . import NEURON

        return NEURON().y3d(i, sec=self)

    def z3d(self, i):
        from . import NEURON

        return NEURON().z3d(i, sec=self)


# ── CachedSegment ─────────────────────────────────────────────────────────────

class CachedSegment:
    """Convenience wrapper that caches ``c_double *`` pointers for a Segment.

    Provides attribute-style access (``cs.v``, ``cs.gnabar_hh = 0.13``)
    over cached range-variable pointers, falling back to the underlying
    Segment for methods (``cs.area()``) and non-range attributes.

    Note: ``cs.v`` is **not** measurably faster than ``seg.v`` because
    Python's attribute-access overhead dominates the ctypes call this
    avoids. For tight loops, call ``seg.ptr('v')`` once and read
    ``ptr[0]`` inline — that path is roughly 15x faster and beats
    real NEURON's CPython extension.

    Staleness: the snapshot is taken at construction. If ``sec.nseg``
    changes (or the section is otherwise reallocated), the next access
    raises ``RuntimeError``. Call ``refresh()`` to re-cache.

    Example::

        cs = soma(0.5).cached()
        cs.v = -65.0          # write through pointer
        soma.nseg = 11
        cs.refresh()
    """

    __slots__ = ("_seg", "_ptrs", "_topo_ver_at_cache", "_nseg_at_cache")

    def __init__(self, segment):
        if not isinstance(segment, Segment):
            raise TypeError(f"expected Segment, got {type(segment).__name__}")
        object.__setattr__(self, "_seg", segment)
        object.__setattr__(self, "_ptrs", {})
        # Snapshot NEURON's internal staleness counters (diam_changed +
        # structure_change_cnt). Comparing the snapshot to the live
        # counters detects any change to topology/diameter from any
        # source — HOC code, real NEURON's Python extension, or external
        # models that call nrn_section_new directly.
        from .api import get_topology_version
        object.__setattr__(self, "_topo_ver_at_cache", get_topology_version())
        # ALSO snapshot nseg directly. The diam_changed/structure_change_cnt
        # counters are updated lazily — they do NOT move when nseg= reallocates
        # the node array or delete_section() frees it *before the first
        # finitialize*, so the counters alone would serve a dangling pointer in
        # that window. nseg reflects the reallocation immediately,
        # and reading it via the property re-checks native liveness.
        object.__setattr__(self, "_nseg_at_cache", segment._sec.nseg)

    @property
    def x(self):
        return self._seg.x

    @property
    def sec(self):
        return self._seg.sec

    def _check_topology(self):
        # nseg first: reading it via the property calls Section._check_alive
        # (native is_active), so a section deleted through HOC raises here, and
        # a comparison mismatch catches an nseg reallocation that the lazy
        # diam/structure counters miss before the first finitialize.
        if self._seg._sec.nseg != self._nseg_at_cache:
            raise RuntimeError(
                "CachedSegment is stale: nseg changed since cache. "
                "Call refresh()."
            )
        # Then the internal counters: catch diam/topology changes from any
        # source (HOC, real NEURON's extension, external nrn_section_new).
        from .api import get_topology_version
        if get_topology_version() != self._topo_ver_at_cache:
            raise RuntimeError(
                "CachedSegment is stale: topology or diameter changed "
                "since cache (nseg, diam, finitialize, or similar). "
                "Call refresh()."
            )

    def _ptr_for(self, name):
        ptr = self._ptrs.get(name)
        if ptr is not None:
            return ptr
        info = _get_symbol_info(name)
        if info is None:
            return None
        sym, sym_type, is_array, _ = info
        # Only scalar range variables are addressable via
        # nrn_rangevar_push. Array range vars and ion globals fall through.
        from .api import TYPES
        if sym_type != TYPES.RANGEVAR or is_array:
            return None
        from .api import _nrn_rangevar_push, _nrn_double_ptr_pop
        try:
            _nrn_rangevar_push(sym, self._seg.sec._sec, ctypes.c_double(self._seg.x))
            raw = _nrn_double_ptr_pop()
        except Exception:
            # Falls back to slow-path getattr on push failure — intentional silent degradation.
            return None
        if not raw:
            return None
        ptr = ctypes.cast(raw, ctypes.POINTER(ctypes.c_double))
        self._ptrs[name] = ptr
        return ptr

    def __getattr__(self, name):
        # Only called when normal attribute lookup misses, so the
        # @property and __slots__ entries are handled before reaching here.
        if name.startswith("_"):
            raise AttributeError(name)
        self._check_topology()
        ptr = self._ptr_for(name)
        if ptr is not None:
            return ptr[0]
        return getattr(self._seg, name)

    def __setattr__(self, name, value):
        if name in CachedSegment.__slots__:
            object.__setattr__(self, name, value)
            return
        self._check_topology()
        ptr = self._ptr_for(name)
        if ptr is not None:
            ptr[0] = float(value)
            return
        setattr(self._seg, name, value)

    def refresh(self):
        """Re-cache pointers after an nseg or topology change."""
        object.__setattr__(self, "_ptrs", {})
        from .api import get_topology_version
        object.__setattr__(self, "_topo_ver_at_cache", get_topology_version())
        object.__setattr__(self, "_nseg_at_cache", self._seg._sec.nseg)

    def __repr__(self):
        return f"<CachedSegment {self._seg!r}>"
