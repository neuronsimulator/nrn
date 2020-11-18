try:
    # Try importing but catch exception in case bindings are not available
    from ._nmodl import NmodlDriver, to_json, to_nmodl  # noqa
    from ._nmodl import __version__
    __all__ = ["NmodlDriver", "to_json", "to_nmodl"]
except ImportError:
    print("[NMODL] [warning] :: Python bindings are not available")
