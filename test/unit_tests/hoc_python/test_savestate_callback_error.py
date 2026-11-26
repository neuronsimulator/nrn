"""A SaveState store callback that raises makes save() raise.

The interface installs the SaveState hooks. A store callback that raises once
crashed the process, when a second registration replaced those hooks.
"""

import neuron
import pytest
from neuron import h


def test_store_callback_error_raises():
    def store():
        raise ValueError("store failed on purpose")

    def restore(data):
        pass

    neuron.register_savestate("test_savestate_callback_error", store, restore)
    try:
        _sec = h.Section(name="savestate_error_sec")  # a model to save
        h.finitialize(-65)
        ss = h.SaveState()
        with pytest.raises(RuntimeError, match="SaveState: Data store failure"):
            ss.save()
    finally:
        index = neuron._id_savestates.index("test_savestate_callback_error")
        del neuron._id_savestates[index]
        del neuron._store_savestates[index]
        del neuron._restore_savestates[index]
