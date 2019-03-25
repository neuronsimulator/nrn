# ***********************************************************************
# Copyright (C) 2018-2019 Blue Brain Project
#
# This file is part of NMODL distributed under the terms of the GNU
# Lesser General Public License. See top-level LICENSE file for details.
# ***********************************************************************

from nmodl.ode import differentiate2c, _make_unique_prefix


def test_make_unique_prefix():

    # if prefix matches start of any var, append underscores
    # to prefix until this is not true
    assert _make_unique_prefix(["a", "b", "ccc", "tmp"], "z") == "z"
    assert _make_unique_prefix(["a", "b", "ccc", "tmp"], "a") == "a_"
    assert _make_unique_prefix(["a", "b", "ccc", "tmp"], "az") == "az"
    assert _make_unique_prefix(["a", "b", "ccc", "tmp"], "cc") == "cc_"
    assert _make_unique_prefix(["a", "b", "ccc", "tmp"], "ccc") == "ccc_"
    assert _make_unique_prefix(["a", "b", "ccc", "tmp"], "tmp") == "tmp_"
    assert _make_unique_prefix(["a", "b", "tmp_", "tmp"], "tmp") == "tmp__"
    assert _make_unique_prefix(["a", "tmp2", "ccc", "x"], "tmp") == "tmp_"
    assert _make_unique_prefix(["a", "tmp2", "ccc", "x"], "tmpvar") == "tmpvar"


def test_differentiation():

    # simple examples, no prev_expressions
    assert differentiate2c("0", "x", "") == "0"
    assert differentiate2c("x", "x", "") == "1.0"
    assert differentiate2c("a", "x", "a") == "0"
    assert differentiate2c("a*x", "x", "a") == "a"
    assert differentiate2c("a*x", "a", "x") == "x"
    assert differentiate2c("a*x", "y", {"x", "y"}) == "0"
    assert differentiate2c("a*x + b*x*x", "x", {"a", "b"}) == "a + 2.0*b*x"
    assert differentiate2c("a*cos(x+b)", "x", {"a", "b"}) == "-a*sin(b + x)"
    assert (
        differentiate2c("a*cos(x+b) + c*x*x", "x", {"a", "b", "c"})
        == "-a*sin(b + x) + 2.0*c*x"
    )

    # single prev_expression to substitute
    assert differentiate2c("a*x + b", "x", {"a", "b", "c", "d"}, ["c = sqrt(d)"]) == "a"
    assert differentiate2c("a*x + b", "x", {"a", "b"}, ["b = 2*x"]) == "a + 2.0"

    # multiple prev_eqs to substitute
    # (these statements should be in the same order as in the mod file)
    assert differentiate2c("a*x + b", "x", {"a", "b", "c"}, ["b = 2*x", "a = 2*x*x*c"]) == "6.0*c*pow(x, 2) + 2.0"
    assert differentiate2c("a*x + b", "x", {"a", "b", "c"}, ["b = 2*x^2", "a = c*cos(x)"]) == "-c*x*sin(x) + c*cos(x) + 4.0*x"

    # multiple prev_eqs to recursively substitute
    # note prev_eqs always substituted in reverse order
    assert differentiate2c("a*x + b", "x", {"a", "b", "c"}, ["a=c*x", "b = 2*a*x"]) == "6.0*c*x"
    assert (
        differentiate2c(
            "a*x + b*c", "x", {"a", "b", "c"}, ["a=3", "b = 2*a*x", "c = a/x"]
        )
        == "a"
    )
    assert (
        differentiate2c("-a*x + b*c", "x", {"a", "b", "c"}, ["b = 2*x*x", "c = a/x"])
        == "a"
    )
    assert (
        differentiate2c(
            "(g1 + g2)*(v-e)",
            "v",
            {"g", "e", "g1", "g2", "c", "d"},
            ["g2 = sqrt(d) + 3", "g1 = 2*c", "g = g1 + g2"],
        )
        == "g"
    )
