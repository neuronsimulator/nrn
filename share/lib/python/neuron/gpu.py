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
    """Runtime configuration for NEURON GPU execution."""

    _VALID_BACKENDS = frozenset({"native", "coreneuron"})

    def __init__(self):
        self._enable = False
        self._backend = "coreneuron"
        self._device_count = 0
        self._permute = None
        self._download_flush_interval = 1

    def __call__(self, **kwargs):
        return GPUContextHelper(self, kwargs)

    def _pc(self):
        from neuron import h

        return h.ParallelContext()

    @property
    def enable(self):
        return self._enable

    @enable.setter
    def enable(self, value):
        new_enable = bool(int(value))
        was_enabled = self._enable
        self._enable = new_enable
        self._sync_to_hoc()
        if new_enable and not was_enabled:
            self._apply_default_permute()

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

    @property
    def device_count(self):
        """Number of GPUs per node (0 = all available)."""
        return self._device_count

    @device_count.setter
    def device_count(self, value):
        self._device_count = int(value)
        self._sync_to_hoc()

    @property
    def download_flush_interval(self):
        """Steps between host downloads during psolve (0 = end of psolve only)."""
        return self._download_flush_interval

    @download_flush_interval.setter
    def download_flush_interval(self, value):
        self._download_flush_interval = int(value)
        self._sync_to_hoc()

    @property
    def permute(self):
        """Cell permutation order (0, 1, or 2). Default 2 when enable is True."""
        if self._permute is None:
            return 2 if self._enable else 0
        return self._permute

    @permute.setter
    def permute(self, value):
        value = int(value)
        if value not in {0, 1, 2}:
            raise ValueError("gpu.permute must be 0, 1, or 2, got {!r}".format(value))
        self._permute = value
        if self._enable:
            self._pc().optimize_node_order(value)

    def _apply_default_permute(self):
        if self._permute is None:
            self._pc().optimize_node_order(2)
        else:
            self._pc().optimize_node_order(self._permute)

    def _sync_to_hoc(self):
        try:
            pc = self._pc()
        except Exception:
            return
        pc.gpu_enable(int(self._enable))
        pc.gpu_backend(self._backend)
        pc.gpu_device_count(int(self._device_count))
        pc.gpu_download_flush_interval(int(self._download_flush_interval))


sys.modules[__name__] = gpu()