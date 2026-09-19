# xvalue/xvarlabel Python bindings call guigetval/guigetstr at create; must not abort.
import os

from neuron import config, h, gui
from neuron.expect_hocerr import expect_err, set_quiet

set_quiet(False)


class Box:
    x = 1.0
    s = "hello"


def have_gui():
    return config.arguments["NRN_ENABLE_INTERVIEWS"] and "DISPLAY" in os.environ


def test_guiget_errors():
    if not have_gui():
        return
    h.xpanel("")
    h.xvalue("ok", (Box(), "x"))
    h.xvarlabel((Box(), "s"))
    expect_err('h.xvalue("n", (Box(), "nope"))')
    expect_err('h.xvalue("n", (Box(), "s"))')
    expect_err('h.xvarlabel((Box(), "nope"))')
    h.xpanel()


if __name__ == "__main__":
    test_guiget_errors()
