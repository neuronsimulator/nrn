import os
import sys

import pytest

from neuron import (
    install_raw_tty_newline_fix,
    _kernel_maps_nl_to_crnl,
    _with_explicit_cr,
)


def test_with_explicit_cr():
    assert _with_explicit_cr("hello\n") == "hello\r\n"
    assert _with_explicit_cr("a\r\nb\n") == "a\r\nb\r\n"
    assert _with_explicit_cr("no newline") == "no newline"


@pytest.mark.skipif(sys.platform == "win32", reason="termios/pty Unix only")
def test_kernel_maps_nl_cooked_vs_raw_pty():
    import termios

    master, slave = os.openpty()
    try:
        attrs = termios.tcgetattr(slave)
        attrs[1] |= termios.OPOST | getattr(termios, "ONLCR", 0)
        termios.tcsetattr(slave, termios.TCSANOW, attrs)
        assert _kernel_maps_nl_to_crnl(slave)

        attrs[1] &= ~termios.OPOST
        termios.tcsetattr(slave, termios.TCSANOW, attrs)
        assert not _kernel_maps_nl_to_crnl(slave)
    finally:
        os.close(master)
        os.close(slave)


@pytest.mark.skipif(sys.platform == "win32", reason="termios/pty Unix only")
def test_patched_write_inserts_cr_only_when_opost_off():
    import termios

    def write_and_read(opost_on):
        master, slave = os.openpty()
        stream = None
        try:
            attrs = termios.tcgetattr(slave)
            if opost_on:
                attrs[1] |= termios.OPOST | getattr(termios, "ONLCR", 0)
            else:
                attrs[1] &= ~termios.OPOST
            termios.tcsetattr(slave, termios.TCSANOW, attrs)
            stream = os.fdopen(slave, "w", buffering=1, closefd=True)
            slave = None
            install_raw_tty_newline_fix(streams=(stream,))
            stream.write("hello\n")
            stream.flush()
            return os.read(master, 64)
        finally:
            if stream is not None:
                stream.close()
            elif slave is not None:
                os.close(slave)
            os.close(master)

    raw = write_and_read(opost_on=False)
    assert raw == b"hello\r\n"

    cooked = write_and_read(opost_on=True)
    assert b"\r\r" not in cooked
    assert cooked.endswith(b"\n")
    assert cooked.startswith(b"hello")
