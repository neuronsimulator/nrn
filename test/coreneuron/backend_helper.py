"""Shared backend selection for CoreNEURON vs native GPU modtests."""
import os

from neuron.tests.utils.strtobool import strtobool


def is_native_backend_test():
    return os.environ.get("NRN_GPU_BACKEND_TEST", "").lower() == "native"


def _set_native_gpu(enable):
    from neuron import h

    h("_nrn_gpu_set_enable(%d)" % int(enable))
    if enable:
        h('_nrn_gpu_set_backend("native")')


def enable_test_backend():
    if is_native_backend_test():
        from neuron import h

        _set_native_gpu(True)
        perm = int(os.environ.get("NRN_GPU_PERMUTE", "2"))
        h.ParallelContext().optimize_node_order(perm)
        return "native"
    from neuron import coreneuron

    coreneuron.enable = True
    coreneuron.gpu = bool(strtobool(os.environ.get("CORENRN_ENABLE_GPU", "false")))
    return "coreneuron"


def disable_test_backend():
    if is_native_backend_test():
        _set_native_gpu(False)
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
        from neuron import h

        h.ParallelContext().optimize_node_order(perm)
    else:
        from neuron import coreneuron

        coreneuron.cell_permute = perm
