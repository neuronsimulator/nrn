"""Mechanism, DensityMechanism, and RangeVar wrappers for mechanism access."""
# Imports from sections and the package stay inside functions (cycle); see
# the note in sections.py.
import ctypes

from .api import (
    _ERR_BUF_SIZE,
    TYPES,
    _nrn_rangevar_get,
    _nrn_rangevar_set,
    _nrn_setpointer_pop,
    _nrn_symbol,
    _nrn_symbol_type as _NRN_SYMBOL_TYPE,
)
from .nrnref import (
    NrnRangeVarRef,
    NrnRef,
)
from .utils import (
    FuncWrapper,
    _try_wrap_density_nmodlrandom,
    list_functions,
)

# Mechanisms that are always present and should be skipped during iteration
# (matches real NEURON's segment iteration behavior).
_SKIP_MECHS = frozenset({"morphology", "capacitance"})


# The model epoch lives in the package __init__, which cannot be imported at
# module top (circular). Bind lazily and cache, so the hot read avoids a
# per-read `from . import`.
_MODEL_EPOCH_FN = None


def _model_epoch_now():
    global _MODEL_EPOCH_FN
    if _MODEL_EPOCH_FN is None:
        from . import _model_epoch

        _MODEL_EPOCH_FN = _model_epoch
    return _MODEL_EPOCH_FN()


# Per-mechanism attribute cache, shared across all instances of the same
# mechanism type. Structure: {mech_name: {attr_name: (full_name, sym, sym_type)}}
# where attr_name is the unsuffixed user-facing name ('gnabar') and
# full_name is the resolved HOC symbol ('gnabar_hh').
#
# WHY: avoids encode + nrn_symbol + nrn_symbol_type on every `seg.hh.gnabar`
# read. Same approach as sections._SYMBOL_INFO_CACHE (real NEURON keeps the
# equivalent pmech_types/rangevars_ dicts at nrnpy_nrn.cpp:3146-3155). With the
# Mechanism instance cache on Segment this takes seg.hh.gnabar from ~10x+
# overhead to ~2-3x (current figures: benchmarks/python/canonical_constants.py).
# INVALIDATES: never. Range-var symbols never move once registered; a later
# nrn_load_dll adds mechanisms without disturbing existing entries.
_MECH_ATTR_CACHE = {}


def _all_density_mechanism_names():
    """Return list of all density mechanism names in registration order,
    excluding morphology and capacitance (matching real NEURON's iteration
    behavior).

    Not cached: nrn_load_dll() can add type-311 symbols at runtime (see
    tests/regression/test_mod_compilation test_custom_mechanism_iteration).
    Section._get_inserted_mechs caches the hot path.
    """
    mechs = [
        (subtype, name)
        for name, (type_, subtype) in list_functions().items()
        if type_ == TYPES.MECHANISM and name not in _SKIP_MECHS
    ]
    mechs.sort()
    return [name for _, name in mechs]


def _array_length(full_name):
    """Array size of a range variable, or 0 for a scalar."""
    from .sections import _get_symbol_info

    info = _get_symbol_info(full_name)
    return info[3] if info is not None and info[2] else 0


class RangeVar:
    """A single range variable on a mechanism at a segment.

    Obtained as e.g. ``seg.hh._ref_gnabar`` for use by HOC functions
    that take a pointer. Behaves like a 1-element double array: read
    via ``rv[0]``, write via ``rv[0] = value``.
    """

    __slots__ = ("_seg", "_full_name")

    def __init__(self, seg, full_name):
        self._seg = seg
        self._full_name = full_name

    def name(self):
        return self._full_name

    def __getitem__(self, idx):
        if idx != 0:
            raise IndexError("Only index 0 is supported")

        # HOC can delete the host section while this ref is held. Reject the
        # read before it dereferences a freed Section*/node and aborts.
        self._seg._sec._check_alive()
        sym = _nrn_symbol(self._full_name.encode("utf-8"))
        from .sections import _is_pointer_rangevar, _pointer_rangevar_get

        if _is_pointer_rangevar(self._full_name, sym):
            return _pointer_rangevar_get(self._seg._sec, self._seg._x, self._full_name)
        return _nrn_rangevar_get(sym, self._seg._sec._sec, self._seg._x)

    def __setitem__(self, idx, value):
        if idx != 0:
            raise IndexError("Only index 0 is supported")

        self._seg._sec._check_alive()  # HOC may have deleted the host section.
        sym = _nrn_symbol(self._full_name.encode("utf-8"))
        from .sections import _is_pointer_rangevar, _pointer_rangevar_set

        if _is_pointer_rangevar(self._full_name, sym):
            _pointer_rangevar_set(self._seg._sec, self._seg._x, self._full_name, value)
            return
        _nrn_rangevar_set(sym, self._seg._sec._sec, self._seg._x, value)

    def __repr__(self):
        return f"<RangeVar {self._full_name}>"


class Mechanism:
    """A density mechanism (e.g. ``hh``, ``pas``) at a specific segment.

    Obtained via ``seg.<mech_name>`` (e.g. ``seg.hh``). Range variables are
    attribute reads/writes (``seg.hh.gnabar``); NMODL ``FUNCTION``/``PROCEDURE``
    blocks are callable methods. ``Mechanism`` is a non-owning wrapper —
    insertion lives on the parent Section (``sec.insert('hh')``).
    """

    __slots__ = ("_seg", "_name", "_is_ion", "_epoch")

    def __init__(self, seg, name):
        self._seg = seg
        self._name = name
        self._is_ion = name.endswith("_ion")
        # Model epoch at which Segment.__getattr__ validated this wrapper as
        # inserted. A later mutation bumps the epoch and __getattr__ revalidates.
        self._epoch = _model_epoch_now()

    def name(self):
        return self._name

    def segment(self):
        """The segment this mechanism belongs to (a method, as in NEURON)."""
        return self._seg

    def is_ion(self):
        return self._is_ion

    def _varnames(self):
        """Return the list of range variable names for this mechanism.

        For non-ion mechanisms (e.g. hh), range variables are suffixed:
        gnabar_hh, m_hh, etc.  For ions (e.g. na_ion), variables follow
        a strict naming convention: eX, Xi, Xo, iX, diX_dv_ where X is
        the ion prefix (e.g. "na" for na_ion).
        """
        all_syms = list_functions()
        names = []

        if self._is_ion:
            # Ion variable naming convention: for ion prefix X
            # eX (reversal), Xi (internal conc), Xo (external conc),
            # iX (current), diX_dv_ (current derivative)
            x = self._name.split("_")[0]  # "na", "k", "ca", etc.
            ion_patterns = {f"e{x}", f"{x}i", f"{x}o", f"i{x}", f"di{x}_dv_"}
            for sym_name, (type_, _) in all_syms.items():
                if type_ == TYPES.RANGEVAR and sym_name in ion_patterns:
                    names.append(sym_name)
        else:
            suffix = f"_{self._name}"
            for sym_name, (type_, _) in all_syms.items():
                if type_ == TYPES.RANGEVAR and sym_name.endswith(suffix):
                    names.append(sym_name)

        return sorted(names)

    def _random_names(self):
        """Return full names of NMODL RANDOM members for this mechanism."""
        if self._is_ion:
            return []

        suffix = f"_{self._name}"
        names = []
        for sym_name, (sym_type, _) in list_functions().items():
            if not sym_name.endswith(suffix):
                continue
            if TYPES.RANGEOBJ is not None:
                if sym_type == TYPES.RANGEOBJ:
                    names.append(sym_name)
                continue
            sym = _nrn_symbol(sym_name.encode("utf-8"))
            wrapped = _try_wrap_density_nmodlrandom(
                self._seg._sec._sec, self._seg._x, sym, sym_type
            )
            if wrapped is not None:
                names.append(sym_name)
        return sorted(names)

    def __getattr__(self, name):
        """Resolve unsuffixed attribute name to a range variable read or an NMODL function wrapper."""
        # HOC can delete the host section while this Mechanism is held; check
        # before touching freed Section*/node storage. Dunder probes
        # (copy/pickle) skip the check and raise AttributeError as usual.
        if not (name.startswith("__") and name.endswith("__")):
            self._seg._sec._check_alive()
            # An uninserted mechanism leaves the section alive but frees its
            # prop, so a range-var read SIGABRTs where real NEURON raises
            # ReferenceError. Revalidate only when the model epoch changed (a
            # cheap int compare); has_membrane is paid once per mutation
            # boundary, not per read. Ions are skipped.
            ep = _model_epoch_now()
            if ep != self._epoch:
                if not self._is_ion and not self._seg._sec.has_membrane(self._name):
                    raise ReferenceError(
                        f"mechanism '{self._name}' is no longer inserted in "
                        f"{self._seg._sec!r}"
                    )
                self._epoch = ep
        # Pointer/reference to a mechanism range variable: seg.brain._ref_V1.
        # Used as the source side of a setpointer (seg.body._ref_PTR = that).
        if name.startswith("_ref_"):
            var = name[5:]
            full = var if self._is_ion else "%s_%s" % (var, self._name)
            from .sections import _is_pointer_rangevar, _pointer_rangevar_get

            sym = _nrn_symbol(full.encode("utf-8"))
            if sym and _is_pointer_rangevar(full, sym):
                _pointer_rangevar_get(
                    self._seg._sec, self._seg._x, full
                )  # raises if unset
            return NrnRangeVarRef(self._seg._sec, self._seg._x, full)
        cache = _MECH_ATTR_CACHE.get(self._name)
        if cache is None:
            cache = {}
            _MECH_ATTR_CACHE[self._name] = cache
        entry = cache.get(name)
        if entry is not None:
            full_name, sym, sym_type = entry
            if sym_type == TYPES.RANGEVAR:
                return _nrn_rangevar_get(sym, self._seg._sec._sec, self._seg._x)
            if sym_type == TYPES.RANGEOBJ:
                wrapped = _try_wrap_density_nmodlrandom(
                    self._seg._sec._sec, self._seg._x, sym, sym_type
                )
                if wrapped is None:
                    raise ReferenceError(
                        f"NMODL RANDOM variable '{full_name}' is no longer live"
                    )
                return wrapped
            # FUN_BLTIN entries are not cached: each call wraps a new closure.
            # Fall through to rebuild it below.
            # TODO(gap-46): if seg.hh.rates(v)-style calls become a bottleneck,
            # cache the (setdata, fn) pair keyed on (mech, name).

        # Map unsuffixed name to the suffixed range variable.
        # For ions: "ena" → "ena" (no suffix)
        # For hh: "gnabar" → "gnabar_hh"
        if self._is_ion:
            full_name = name
        else:
            full_name = f"{name}_{self._name}"

        sym = _nrn_symbol(full_name.encode("utf-8"))
        if sym:
            sym_type = int(_NRN_SYMBOL_TYPE(sym))
            if sym_type == TYPES.RANGEVAR:
                size = _array_length(full_name)
                if size:
                    # seg.mech.c[i]: the scalar getter throws inside the core
                    # for an array variable, so never cache it as a scalar.
                    from .sections import _ArrayRangeVar

                    return _ArrayRangeVar(self._seg._sec, self._seg._x, full_name, size)
                from .sections import _is_pointer_rangevar, _pointer_rangevar_get

                if _is_pointer_rangevar(full_name, sym):
                    # Never cached: the fast path would read it unchecked.
                    return _pointer_rangevar_get(
                        self._seg._sec, self._seg._x, full_name
                    )
                cache[name] = (full_name, sym, sym_type)
                return _nrn_rangevar_get(sym, self._seg._sec._sec, self._seg._x)
            if sym_type == TYPES.FUN_BLTIN and not self._is_ion:
                # NMODL PROCEDURE and FUNCTION both compile to FUN_BLTIN (280),
                # not PROCEDURE (271, HOC-only `proc`); the HOC name is
                # `<name>_<mech>`. The function needs its mechanism bound to a
                # segment first via nmodl's setdata_<mech>, so wrap both:
                # hh.rates(v) instead of setdata_hh(x, sec=sec) + rates_hh(sec=sec).
                seg = self._seg
                mech = self._name
                qualified = f"{mech}.{name}"

                def mech_func(*args):
                    from . import NEURON

                    n = NEURON()
                    setdata = getattr(n, f"setdata_{mech}")
                    setdata(seg._x, sec=seg._sec)
                    fn = getattr(n, full_name)
                    return fn(*args, sec=seg._sec)

                return FuncWrapper(
                    mech_func,
                    qualified,
                    f"NEURON mechanism function: {qualified}()",
                )

            # RANGEOBJ: see api.TypeCodes.record_rangeobj. Cached like a RANGEVAR.
            if TYPES.RANGEOBJ is None or sym_type == TYPES.RANGEOBJ:
                wrapped = _try_wrap_density_nmodlrandom(
                    self._seg._sec._sec, self._seg._x, sym, sym_type
                )
                if wrapped is not None:
                    cache[name] = (full_name, sym, TYPES.RANGEOBJ)
                    return wrapped

        raise AttributeError(f"'{self._name}' mechanism has no variable '{name}'")

    def _func_names(self):
        """Return list of NMODL PROCEDURE/FUNCTION names exposed by this mech."""
        if self._is_ion:
            return []

        suffix = f"_{self._name}"
        # Skip setdata helper — it's an internal nmodl entry, not a user-facing
        # procedure, and shouldn't appear in dir(hh).
        skip = {f"setdata_{self._name}"}
        names = []

        for sym_name, (type_, _) in list_functions().items():
            if (
                type_ == TYPES.FUN_BLTIN
                and sym_name.endswith(suffix)
                and sym_name not in skip
            ):
                names.append(sym_name[: -len(suffix)])
        return sorted(names)

    def _assign_pointer(self, member, src_ref):
        """Wire this density mechanism's NMODL POINTER ``member`` at this
        segment to the address referenced by ``src_ref`` (implements
        ``seg.mech._ref_PTR = src._ref_X``).

        Uses the public ``nrn_setpointer_pop`` API (nrn#3828), which consumes
        exactly one source handle.
        """
        seg = self._seg
        target = "%s_%s" % (member, self._name)
        sym = _nrn_symbol(target.encode("utf-8"))
        if not sym:
            raise NameError("no such density POINTER: %s" % target)

        # Fallible values are prepared before pushing; the C API consumes the
        # source handle on success and failure.
        err = ctypes.create_string_buffer(_ERR_BUF_SIZE)
        src_ref._push()
        rc = _nrn_setpointer_pop(
            sym,
            seg._sec._sec,
            ctypes.c_double(seg._x),
            err,
            _ERR_BUF_SIZE,
        )
        if rc != 0:
            raise RuntimeError(
                "setpointer %s failed: %s"
                % (target, err.value.decode("utf-8", "replace"))
            )

    def __setattr__(self, name, value):
        """Write unsuffixed range variable on the mechanism at the current segment; populates cache."""
        # POINTER assignment: seg.mech._ref_PTR = src._ref_X.
        if name.startswith("_ref_"):
            if isinstance(value, NrnRef):
                self._seg._sec._check_alive()  # HOC may have deleted the host.
                self._assign_pointer(name[5:], value)
                return
        if name.startswith("_"):
            super().__setattr__(name, value)
            return

        # The write below dereferences the host section; raise cleanly if HOC
        # deleted it.
        self._seg._sec._check_alive()
        cache = _MECH_ATTR_CACHE.get(self._name)
        if cache is not None:
            entry = cache.get(name)
            if entry is not None:
                full_name, sym, sym_type = entry
                if sym_type == TYPES.RANGEVAR:
                    _nrn_rangevar_set(sym, self._seg._sec._sec, self._seg._x, value)
                    return
                if sym_type == TYPES.RANGEOBJ:
                    raise ValueError(
                        f"NMODL RANDOM variable '{full_name}' is not assignable"
                    )

        if self._is_ion:
            full_name = name
        else:
            full_name = f"{name}_{self._name}"

        sym = _nrn_symbol(full_name.encode("utf-8"))
        sym_type = int(_NRN_SYMBOL_TYPE(sym)) if sym else None
        if sym and sym_type == TYPES.RANGEVAR:
            if _array_length(full_name):
                # Like NEURON, seg.mech.c = x on an array variable sets c[0].
                from .sections import _ArrayRangeVar

                _ArrayRangeVar(self._seg._sec, self._seg._x, full_name, 1)[0] = value
                return
            from .sections import _is_pointer_rangevar, _pointer_rangevar_set

            if _is_pointer_rangevar(full_name, sym):
                _pointer_rangevar_set(self._seg._sec, self._seg._x, full_name, value)
                return
            if cache is None:
                cache = {}
                _MECH_ATTR_CACHE[self._name] = cache
            cache[name] = (full_name, sym, TYPES.RANGEVAR)
            _nrn_rangevar_set(sym, self._seg._sec._sec, self._seg._x, value)
            return

        if sym and (TYPES.RANGEOBJ is None or sym_type == TYPES.RANGEOBJ):
            wrapped = _try_wrap_density_nmodlrandom(
                self._seg._sec._sec, self._seg._x, sym, sym_type
            )
            if wrapped is not None:
                if cache is None:
                    cache = {}
                    _MECH_ATTR_CACHE[self._name] = cache
                cache[name] = (full_name, sym, TYPES.RANGEOBJ)
                raise ValueError(
                    f"NMODL RANDOM variable '{full_name}' is not assignable"
                )

        raise AttributeError(f"'{self._name}' mechanism has no variable '{name}'")

    def __iter__(self):
        """Iterate over RangeVar objects for this mechanism."""
        for full_name in self._varnames():
            yield RangeVar(self._seg, full_name)

    def __dir__(self):
        names = set()
        # Range variables: hh exposes "gnabar", "m", etc. (unsuffixed)
        for full_name in self._varnames():
            if self._is_ion:
                names.add(full_name)
            else:
                suffix = f"_{self._name}"
                if full_name.endswith(suffix):
                    names.add(full_name[: -len(suffix)])
                else:
                    names.add(full_name)
        # NMODL PROCEDURE/FUNCTION names exposed by the mechanism
        names.update(self._func_names())
        suffix = f"_{self._name}"
        for full_name in self._random_names():
            names.add(full_name[: -len(suffix)])
        # Python-side helpers
        names.update(["name", "is_ion", "segment"])
        return sorted(names)

    def __repr__(self):
        return f"{self._seg}.{self._name}"


class DensityMechanism:
    """Represents a density mechanism type (e.g. hh, pas).

    Returned by n.hh, n.pas, etc. Can be used with Section.insert():
        soma.insert(n.hh)   # equivalent to soma.insert('hh')
    """

    __slots__ = ("_name",)

    def __init__(self, name):
        self._name = name

    @property
    def name(self):
        return self._name

    def insert(self, secs):
        """Insert this mechanism into a section or iterable of sections."""
        from .sections import Section

        if isinstance(secs, Section):
            secs.insert(self._name)
        else:
            for sec in secs:
                sec.insert(self._name)

    def uninsert(self, secs):
        """Remove this mechanism from a section or iterable of sections."""
        from .sections import Section

        if isinstance(secs, Section):
            secs.uninsert(self._name)
        else:
            for sec in secs:
                sec.uninsert(self._name)

    def __repr__(self):
        return self._name

    def __str__(self):
        return self._name
