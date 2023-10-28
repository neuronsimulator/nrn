from neuron import h
sf = h.StringFunctions()

def test_substr():
    assert(sf.substr("foobarfoo", "bar") == 3)
    assert(sf.substr("foobarfoo", "abc") == -1)
    assert(sf.substr("foobarfoo", "foo") == 0)

def test_len():
    assert(sf.len("foobarfoo") == 9)
    assert(sf.len("") == 0)

def test_head():
    pattern = "f.o"
    text = "foobarshi"
    head = h.ref("")
    assert(sf.head(text, pattern, head) == 0)
    assert(head[0] == "")

    pattern = "b.*$"
    text = "foobarshi"
    head = h.ref("")
    assert(sf.head(text, pattern, head) == 3)
    assert(head[0] == "foo")

    pattern = "abc"
    text = "foobarshi"
    head = h.ref("")
    assert(sf.head(text, pattern, head) == -1)
    assert(head[0] == "")

def test_tail():
    pattern = "s.i$"
    text = "foobarshi"
    tail = h.ref("")
    assert(sf.tail(text, pattern, tail) == 9)
    assert(tail[0] == "")

    pattern = "^.*r"
    text = "foobarshi"
    tail = h.ref("")
    assert(sf.tail(text, pattern, tail) == 6)
    assert(tail[0] == "shi")

    pattern = "abc"
    text = "foobarshi"
    tail = h.ref("")
    assert(sf.tail(text, pattern, tail) == -1)
    assert(tail[0] == "")

def test_right():
    s = h.ref("foobarshi")
    sf.right(s, 6)
    assert(s[0] == "shi")
    s = h.ref("foobarshi")
    sf.right(s, 0)
    assert(s[0] == "foobarshi")
    # Out of range
    # s = h.ref("foobarshi")
    # sf.right(s, 10)
    # assert(s[0] == "foobarshi")

def test_left():
    s = h.ref("foobarshi")
    sf.left(s, 3)
    assert(s[0] == "foo")
    s = h.ref("foobarshi")
    sf.left(s, 0)
    assert(s[0] == "")
    # Out of range
    # s = h.ref("foobarshi")
    # sf.left(s, 10)
    # assert(s[0] == "foo")

def test_is_name():
    assert(sf.is_name("xvalue"))
    assert(not sf.is_name("xfoo"))

def test_alias():
    v = h.Vector()
    sf.alias(v, "xv", h.xvalue)
    assert(v.xv == h.xvalue)

def test_alias_list():
    h.load_file('stdrun.hoc')
    v = h.Vector()
    assert(len(sf.alias_list(v)) == 0)
    sf.alias(v, "xv1", h.xvalue) # Add alias
    assert(len(sf.alias_list(v)) == 1)
    sf.alias(v, "xv2", h.xvalue) # Add alias
    assert(len(sf.alias_list(v)) == 2)
    sf.alias(v, "xv1") # Remove 1 alias
    assert(len(sf.alias_list(v)) == 1)
    sf.alias(v) # Remove all
    assert(len(sf.alias_list(v)) == 0)

def test_references():
    # This function print the number of references
    pass

def test_is_point_process():
    pass

def test_is_artificial():
    pass
