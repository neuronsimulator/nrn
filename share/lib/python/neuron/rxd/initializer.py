import itertools

# TODO: monkey-patch everything that requires an init so only even attempts once

def _do_ion_register():
    from . import species
    for obj in species._all_species:
        obj = obj()
        if obj is not None:
            obj._ion_register()


has_initialized = False
def _do_init():
    global has_initialized
    if has_initialized: return
    from . import species, region, rxd
    if len(species._all_species) > 0:
        has_initialized = True
        for obj in itertools.chain(region._all_regions, species._all_species, rxd._all_reactions):
            obj = obj()
            if obj is not None:
                obj._do_init()
        rxd._init()

def is_initialized():
    return has_initialized

def assert_initialized(msg=''):
    from . import species
    if not has_initialized and len(species._all_species) > 0:
        if len(msg):
            msg = ': ' + msg
        from .rxdException import RxDException
        raise RxDException('invalid operation; rxd module not initialized' + msg)
