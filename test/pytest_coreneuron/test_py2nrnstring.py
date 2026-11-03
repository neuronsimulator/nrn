from neuron import h
from neuron.expect_hocerr import expect_hocerr, expect_err, set_quiet
import sys

quiet = True

uni = "ab\xe0"


def checking(s):
    if not quiet:
        print("CHECKING: " + s)


class Foo:  # for testing section name errors when full cell.section name has unicode chars.
    def __init__(self, name):
        self.name = name

    def __str__(self):
        return self.name


def test_py2nrnstring():
    print(uni)

    # I don't think h.getstr() can be made to work from python
    # so can't test unicode on stdin read by hoc.
    h("strdef s")
    h('s = "hello"')
    checking("h('getstr(s)')")
    h("getstr(s)")
    "goodbye"
    assert h.s == "hello"

    checking("h(uni + ' = 1')")
    assert h(uni + " = 1") == 0

    # Allowed! The s format for PyArg_ParseTuple specifies that
    # Unicode objects are converted to C strings using 'utf-8' encoding.
    # If this conversion fails, a UnicodeError is raised.
    checking("""h('s = "%s"'%uni)""")
    assert h('s = "%s"' % uni) == 1
    assert h.s == uni

    checking('h.printf("%s", uni)')
    expect_hocerr(h.printf, ("%s", uni))

    checking("hasattr(h, uni)")
    expect_hocerr(hasattr, (h, uni))

    expect_err("h.à = 1")

    expect_err("a = h.à")

    expect_err('a = h.ref("à")')

    ns = h.NetStim()
    # nonsense but it does test the unicode error message
    checking('h.setpointer(ns._ref_start, "à", ns)')
    expect_hocerr(h.setpointer, (ns._ref_start, "à", ns))
    del ns

    # No error for these two (unless cell is involved)!
    checking("s = h.Section(name=uni)")
    s = h.Section(name=uni)
    assert s.name() == uni
    checking("s = h.Section(uni)")
    s = h.Section(uni)
    assert s.name() == uni

    expect_err('h.Section(name="apical", cell=Foo(uni))')

    soma = h.Section()
    expect_err("a = soma.à")

    expect_err("soma.à = 1")

    expect_err("a = soma(.5).à")

    expect_err("soma(.5).à = 1")

    soma.insert("hh")
    expect_err("a = soma(.5).hh.à")

    expect_err("soma(.5).hh.à = 1")


def test_getstr_word():
    import tempfile

    words = "a space separated   series of words \n\n goodbye"
    wordlist = words.split()
    with tempfile.NamedTemporaryFile(mode="w", delete=True, suffix=".txt") as temp_file:
        temp_file.write(words)
        temp_file.flush()
        # print(f"temp_file {temp_file.name}")

        h.ropen(temp_file.name)
        word = h.ref("")
        for i in range(100):
            h.getstr(word, 1)
            assert word[0] == wordlist[i]
            if word[0] == "goodbye":
                break
        h.ropen()


if __name__ == "__main__":
    set_quiet(False)
    quiet = False
    test_py2nrnstring()
    test_getstr_word()
