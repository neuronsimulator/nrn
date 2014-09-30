import itertools

# TODO: monkey-patch everything that requires an init so only even attempts once

has_initialized = False
def _do_init():
    global has_initialized
    if has_initialized: return
    from . import species, region, rxd
    if len(region._all_regions) > 0:
        has_initialized = True
        for obj in itertools.chain(region._all_regions, species._all_species, rxd._all_reactions):
            obj = obj()
            if obj is not None:
                obj._do_init()

def is_initialized():
    return has_initialized

def assert_initialized(msg=''):
    if not has_initialized:
        if len(msg):
            msg = ': ' + msg
        from .rxdException import RxDException
        raise RxDException('invalid operation; rxd module not initialized' + msg)
