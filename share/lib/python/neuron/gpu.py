import sys


class GPUContextHelper(object):
    def __init__(self, gpu_module, new_values):
        self._gpu = gpu_module
        self._new_values = new_values
        self._old_values = None

    def __enter__(self):
        assert self._new_values is not None
        assert self._old_values is None
        self._old_values = {}
        for key, value in self._new_values.items():
            self._old_values[key] = getattr(self._gpu, key)
            setattr(self._gpu, key, value)

    def __exit__(self, exc_type, exc_val, exc_tb):
        assert self._new_values is not None
        assert self._old_values is not None
        for key in reversed(self._new_values.keys()):
            setattr(self._gpu, key, self._old_values[key])
        return False


class gpu(object):
    """Runtime configuration for NEURON native GPU execution."""

    _VALID_BACKENDS = frozenset({"native", "coreneuron"})

    def __init__(self):
        self._enable = False
        self._backend = "coreneuron"

    def __call__(self, **kwargs):
        return GPUContextHelper(self, kwargs)

    @property
    def enable(self):
        return self._enable

    @enable.setter
    def enable(self, value):
        self._enable = bool(int(value))
        self._sync_to_hoc()

    @property
    def backend(self):
        return self._backend

    @backend.setter
    def backend(self, value):
        backend = str(value).lower()
        if backend not in self._VALID_BACKENDS:
            raise ValueError(
                "gpu.backend must be 'native' or 'coreneuron', got {!r}".format(value)
            )
        self._backend = backend
        self._sync_to_hoc()

    def _sync_to_hoc(self):
        try:
            from neuron import h
        except Exception:
            return
        h("_nrn_gpu_set_enable(%d)" % int(self._enable))
        h('_nrn_gpu_set_backend("%s")' % self._backend)


sys.modules[__name__] = gpu()
