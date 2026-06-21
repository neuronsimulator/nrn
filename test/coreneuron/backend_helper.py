"""Shared backend selection for CoreNEURON vs native GPU modtests."""
import os

from neuron.tests.utils.strtobool import strtobool


def is_native_backend_test():
    return os.environ.get("NRN_GPU_BACKEND_TEST", "").lower() == "native"


def _native_gpu_module():
    try:
        from neuron import gpu
    except ImportError:
        import neuron.gpu as gpu
    return gpu


def enable_test_backend():
    if is_native_backend_test():
        from neuron import h

        gpu = _native_gpu_module()
        gpu.enable = True
        gpu.backend = "native"
        perm = int(os.environ.get("NRN_GPU_PERMUTE", "2"))
        gpu.permute = perm
        return "native"
    from neuron import coreneuron

    coreneuron.enable = True
    coreneuron.gpu = bool(strtobool(os.environ.get("CORENRN_ENABLE_GPU", "false")))
    return "coreneuron"


def disable_test_backend():
    if is_native_backend_test():
        _native_gpu_module().enable = False
        return
    from neuron import coreneuron

    coreneuron.enable = False


def iter_permute_values():
    if is_native_backend_test():
        yield int(os.environ.get("NRN_GPU_PERMUTE", "2"))
        return
    from neuron import coreneuron

    for perm in coreneuron.valid_cell_permute():
        yield perm


def set_permute(perm):
    if is_native_backend_test():
        _native_gpu_module().permute = perm
    else:
        from neuron import coreneuron

        coreneuron.cell_permute = perm
