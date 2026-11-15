from .._interface.api import HOC_LOCK

# TODO: monkey-patch everything that requires an init so only even attempts once


def _do_ion_register() -> None:
    from . import species

    for obj in species._all_species:
        obj = obj()
        if obj is not None:
            obj._ion_register()


# The interface's HOC lock, not a lock of rxd's own. The GUI thread holds the
# HOC lock while doNotify runs, and NEURON can call _setup_matrices from inside
# it, which takes this lock; Species.__init__ takes this lock and then makes HOC
# calls. With two locks those orders deadlocked.
_init_lock = HOC_LOCK
has_initialized: bool = False
is_initializing: bool = False


def _do_init() -> None:
    global has_initialized, is_initializing, _init_lock
    with _init_lock:
        if not has_initialized and not is_initializing:
            from . import species, region, rxd

            if len(species._all_species) > 0:
                is_initializing = True
                # TODO: clean this up so not repetitive; can't do it super cleanly because of the multiple phases of species
                for obj in region._all_regions:
                    obj = obj()
                    if obj is not None:
                        obj._do_init()
                # NOTE: for species, must do all _do_init2s (1D alloc) before _do_init3s (3D alloc) to keep nodes arranged nicely
                for obj in species._all_species:
                    obj = obj()
                    if obj is not None:
                        obj._do_init1()
                for obj in species._all_species:
                    obj = obj()
                    if obj is not None:
                        obj._do_init2()
                for obj in species._all_species:
                    obj = obj()
                    if obj is not None:
                        obj._do_init3()
                for obj in species._all_species:
                    obj = obj()
                    if obj is not None:
                        obj._do_init4()
                for obj in species._all_species:
                    obj = obj()
                    if obj is not None:
                        obj._do_init5()
                for obj in rxd._all_reactions:
                    obj = obj()
                    if obj is not None:
                        obj._do_init()
                rxd._init()
                has_initialized = True
            is_initializing = False


def is_initialized() -> bool:
    return has_initialized


def assert_initialized(msg: str = "") -> None:
    from . import species

    if not has_initialized and len(species._all_species) > 0:
        if len(msg):
            msg = ": " + msg
        from .rxdException import RxDException

        raise RxDException(f"invalid operation; rxd module not initialized{msg}")
