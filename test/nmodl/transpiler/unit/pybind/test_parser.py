import io
from pathlib import Path

import pytest
from neuron import nmodl

# 1,025 copies of é are 2,050 UTF-8 bytes. parse_stream used to copy that
# into a buffer sized by the character count.
_ACCENTS = "é" * 1025


def _comment(body):
    return f"COMMENT\n{body}\nENDCOMMENT\n"


def _round_trip(stream):
    parsed = nmodl.NmodlDriver().parse_stream(stream)
    return nmodl.to_nmodl(parsed)


class _LimitedText(io.TextIOBase):
    """Text read() that returns at most `limit` characters and ignores a smaller request."""

    def __init__(self, text, limit):
        super().__init__()
        self._text = text
        self._limit = limit
        self._at = 0

    def readable(self):
        return True

    def read(self, n=-1):
        if self._at >= len(self._text):
            return ""
        take = min(self._limit, len(self._text) - self._at)
        part = self._text[self._at : self._at + take]
        self._at += len(part)
        return part


class _LimitedBytes:
    """Byte read() that is not a TextIOBase, so parse_stream takes the bytes branch."""

    def __init__(self, data, limit):
        self._data = data
        self._limit = limit
        self._at = 0

    def read(self, n=-1):
        if self._at >= len(self._data):
            return b""
        take = min(self._limit, len(self._data) - self._at)
        part = self._data[self._at : self._at + take]
        self._at += len(part)
        return part


def test_parse_directory():
    """
    Make sure we raise an error when parsing a directory instead of a file
    """

    with pytest.raises(RuntimeError):
        nmodl.NmodlDriver().parse_file(str(Path(__file__).parent))


def test_parse_stream_stringio_utf8():
    """StringIO honors the requested character count. Those characters still expand."""
    assert _round_trip(io.StringIO(_comment(_ACCENTS))).count("é") == 1025


@pytest.mark.parametrize("limit", [1, 1025])
def test_parse_stream_text_keeps_encoded_tail(limit):
    """One-character reads refill the put-back area. A 1,025-character read leaves a tail."""
    assert _round_trip(_LimitedText(_comment(_ACCENTS), limit)).count("é") == 1025


def test_parse_stream_bytes_keeps_unread_tail():
    """A bytes read() may return more bytes than were requested."""
    raw = _comment(_ACCENTS).encode("utf-8")
    assert _round_trip(_LimitedBytes(raw, len(raw))).count("é") == 1025
