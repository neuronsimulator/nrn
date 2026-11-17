"""Collecting a section while an FInitializeHandler raises must not abort.

Formatting the handler's Python exception can collect a section. Node storage
must not be frozen across handler types 0, 1, and 2, or
owning_identifier::destroy aborts when it erases that section's node. Type 3
runs before the token is taken.
"""

import gc

import pytest
from neuron import h


def _section_names():
    return {sec.name() for sec in h.allsec()}


@pytest.mark.parametrize("handler_type", [3, 0, 1, 2], ids=lambda t: f"type{t}")
def test_raising_finitialize_handler_can_collect_a_section(handler_type):
    victim_name = f"victim{handler_type}"
    holder = [h.Section(name=victim_name)]

    class CollectsSection(RuntimeError):
        def __str__(self):
            # neuron.format_exception calls this from nrnpyerr_str, still
            # inside nrn_fihexec.
            holder.clear()
            gc.collect()
            return f"fih-raise-{handler_type}"

    def callback():
        raise CollectsSection(f"fih-raise-{handler_type}")

    fih = h.FInitializeHandler(handler_type, callback)
    try:
        with pytest.raises(RuntimeError, match=f"fih-raise-{handler_type}"):
            h.finitialize()
        assert victim_name not in _section_names()
    finally:
        del fih
        holder.clear()
        gc.collect()

    alive = h.Section(name=f"alive{handler_type}")
    h.finitialize(-65)
    assert alive(0.5).v == -65
    h.fadvance()
    h.delete_section(sec=alive)
