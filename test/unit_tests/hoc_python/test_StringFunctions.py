from neuron import h
from neuron.expect_hocerr import expect_err

sf = h.StringFunctions()


def test_substr():
    assert sf.substr("foobarfoo", "bar") == 3
    assert sf.substr("foobarfoo", "abc") == -1
    assert sf.substr("foobarfoo", "foo") == 0


def test_len():
    assert sf.len("foobarfoo") == 9
    assert sf.len("") == 0


def test_head():
    pattern = "f.o"
    text = "foobarshi"
    head = h.ref("")
    assert sf.head(text, pattern, head) == 0
    assert head[0] == ""

    pattern = "b.*$"
    text = "foobarshi"
    head = h.ref("")
    assert sf.head(text, pattern, head) == 3
    assert head[0] == "foo"

    pattern = "abc"
    text = "foobarshi"
    head = h.ref("")
    assert sf.head(text, pattern, head) == -1
    assert head[0] == ""


def test_tail():
    pattern = "s.i$"
    text = "foobarshi"
    tail = h.ref("")
    assert sf.tail(text, pattern, tail) == 9
    assert tail[0] == ""

    pattern = "^.*r"
    text = "foobarshi"
    tail = h.ref("")
    assert sf.tail(text, pattern, tail) == 6
    assert tail[0] == "shi"

    pattern = "abc"
    text = "foobarshi"
    tail = h.ref("")
    assert sf.tail(text, pattern, tail) == -1
    assert tail[0] == ""


def text_rtrim():
    text = "bar\t; \t\n"
    out = h.ref("")
    sf.rtrim(text, out)
    assert out[0] == "bar\t;"

    sf.rtrim(text, out, " \t\n\f\v\r;")
    assert out[0] == "bar"


def test_ltrim():
    text = "  \t \n# foo"
    out = h.ref("")
    sf.ltrim(text, out)
    assert out[0] == "# foo"

    sf.ltrim(text, out, " \t\n\f\r\v#")
    assert out[0] == "foo"


def test_right():
    s = h.ref("foobarshi")
    sf.right(s, 6)
    assert s[0] == "shi"
    s = h.ref("foobarshi")
    sf.right(s, 0)
    assert s[0] == "foobarshi"
    # Out of range
    # s = h.ref("foobarshi")
    # sf.right(s, 10)
    # assert(s[0] == "foobarshi")


def test_left():
    s = h.ref("foobarshi")
    sf.left(s, 3)
    assert s[0] == "foo"
    s = h.ref("foobarshi")
    sf.left(s, 0)
    assert s[0] == ""
    # Out of range
    # s = h.ref("foobarshi")
    # sf.left(s, 10)
    # assert(s[0] == "foo")


def test_is_name():
    assert sf.is_name("xvalue")
    assert not sf.is_name("xfoo")


def test_alias():
    v = h.Vector()
    sf.alias(v, "xv", h.xvalue)
    assert v.xv == h.xvalue
    h("""double x[2]""")
    sf.alias(v, "xy", h._ref_x[0])
    v.xy = 3.14
    assert h.x[0] == v.xy


def test_alias_list():
    v = h.Vector()
    expect_err("sf.alias_list(v)")  # no hoc String template
    # after an expect error, must manually delete
    del v
    assert len(locals()) == 0  #  sonarcloud says return value must be used

    v = h.Vector()
    h.load_file("stdrun.hoc")
    assert len(sf.alias_list(v)) == 0
    sf.alias(v, "xv1", h.xvalue)  # Add alias
    assert len(sf.alias_list(v)) == 1
    sf.alias(v, "xv2", h.xvalue)  # Add alias
    assert len(sf.alias_list(v)) == 2
    sf.alias(v, "xv1")  # Remove 1 alias
    assert len(sf.alias_list(v)) == 1
    sf.alias(v)  # Remove all
    assert len(sf.alias_list(v)) == 0


def test_references():
    # This function prints the number of references
    sf.references(None)
    v = h.Vector()

    # different ways a hoc object can be referenced
    h.hoc_obj_[0] = v
    l = h.List()
    l.append(v)
    h(
        """
objref o
begintemplate Foo
public o, o2
objref o, o2[2]
proc init() {
    o = $o1
    o2[0] = o
}
endtemplate Foo
    """
    )
    foo = h.Foo(v)
    h.o = v
    box = h.VBox()
    box.ref(v)

    sf.references(foo.o)
    box.ref(None)  # without this, then assert error when box is destroyed


def test_is_point_process():
    sf = h.StringFunctions()  # no destructor (removed a non-coverable line)
    s = h.Section()
    assert not sf.is_artificial(h.List())
    assert sf.is_point_process(h.IClamp(s(0.5)))
    assert sf.is_point_process(h.NetStim())


def test_is_artificial():
    s = h.Section()
    assert not sf.is_artificial(h.List())
    assert not sf.is_artificial(h.IClamp(s(0.5)))
    assert sf.is_artificial(h.NetStim())
