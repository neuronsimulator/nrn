# ***********************************************************************
# Copyright (C) 2018-2022 Blue Brain Project
#
# This file is part of NMODL distributed under the terms of the GNU
# Lesser General Public License. See top-level LICENSE file for details.
# ***********************************************************************

from nmodl.ode import differentiate2c, integrate2c

import sympy as sp


def _equivalent(
    lhs, rhs, vars=["a", "b", "c", "d", "e", "f", "v", "w", "x", "y", "z", "t", "dt"]
):
    """Helper function to test equivalence of analytic expressions
    Analytic expressions can often be written in many different,
    but mathematically equivalent ways. This helper function uses
    SymPy to check if two analytic expressions are equivalent.
    If the expressions contain an "=", each is split into two expressions,
    and the two pairs of expressions are compared, i.e.
    _equivalent("a=b+c", "a=c+b") is the same thing as doing
    _equivalent("a", "a") and _equivalent("b+c", "c+b")
    Args:
        lhs: first expression, e.g. "x*(1-a)"
        rhs: second expression, e.g. "-a*x + x"
        vars: list of variables used in expressions, e.g. ["a", "x"]
    Returns:
        True if expressions are equivalent, False if they are not
    """
    lhs = lhs.replace("pow(", "Pow(")
    rhs = rhs.replace("pow(", "Pow(")
    sympy_vars = {var: sp.symbols(var, real=True) for var in vars}
    for l, r in zip(lhs.split("=", 1), rhs.split("=", 1)):
        eq_l = sp.sympify(l, locals=sympy_vars)
        eq_r = sp.sympify(r, locals=sympy_vars)
        difference = (eq_l - eq_r).evalf().simplify()
        if difference != 0:
            return False
    return True


def test_differentiate2c():

    # simple examples, no prev_expressions
    assert _equivalent(differentiate2c("0", "x", ""), "0")
    assert _equivalent(differentiate2c("x", "x", ""), "1")
    assert _equivalent(differentiate2c("a", "x", "a"), "0")
    assert _equivalent(differentiate2c("a*x", "x", "a"), "a")
    assert _equivalent(differentiate2c("a*x", "a", "x"), "x")
    assert _equivalent(differentiate2c("a*x", "y", {"x", "y"}), "0")
    assert _equivalent(differentiate2c("a*x + b*x*x", "x", {"a", "b"}), "2*b*x+a")
    assert _equivalent(
        differentiate2c("a*cos(x+b)", "x", {"a", "b"}), "-a * sin(b + x)"
    )
    assert _equivalent(
        differentiate2c("a*cos(x+b) + c*x*x", "x", {"a", "b", "c"}),
        "-a*sin(b+x) + 2*c*x",
    )

    # single prev_expression to substitute
    assert _equivalent(
        differentiate2c("a*x + b", "x", {"a", "b", "c", "d"}, ["c = sqrt(d)"]), "a"
    )
    assert _equivalent(
        differentiate2c("a*x + b", "x", {"a", "b"}, ["b = 2*x"]), "a + 2"
    )

    # multiple prev_eqs to substitute
    # (these statements should be in the same order as in the mod file)
    assert _equivalent(
        differentiate2c("a*x + b", "x", {"a", "b"}, ["b = 2*x", "a = -2*x*x"]), "-6*x*x+2"
    )
    assert _equivalent(
        differentiate2c("a*x + b", "x", {"a", "b"}, ["b = 2*x*x", "a = -2*x"]), "0"
    )

    # multiple prev_eqs to recursively substitute
    # note prev_eqs always substituted in reverse order
    # and only x-dependent rhs's are substituted, e.g. 'a' remains 'a' here:
    assert _equivalent(
        differentiate2c("a*x + b", "x", {"a", "b"}, ["a=3", "b = 2*a*x"]), "3*a"
    )
    assert _equivalent(
        differentiate2c(
            "a*x + b*c", "x", {"a", "b", "c"}, ["a=3", "b = 2*a*x", "c = a/x"]
        ),
        "a",
    )
    assert _equivalent(
        differentiate2c("-a*x + b*c", "x", {"a", "b", "c"}, ["b = 2*x*x", "c = a/x"]),
        "a",
    )
    assert _equivalent(
        differentiate2c(
            "(g1 + g2)*(v-e)",
            "v",
            {"g", "e", "g1", "g2", "c", "d"},
            ["g2 = sqrt(d) + 3", "g1 = 2*c", "g = g1 + g2"],
        ),
        "g",
    )


def test_integrate2c():

    # list of variables used for integrate2c
    var_list = ["x", "a", "b"]
    # pairs of (f(x), g(x))
    # where f(x) is the differential equation: dx/dt = f(x)
    # and g(x) is the solution: x(t+dt) = g(x(t))
    test_cases = [
        ("0", "x"),
        ("a", "x + a*dt"),
        ("a*x", "x*exp(a*dt)"),
        ("a*x+b", "(-b + (a*x + b)*exp(a*dt))/a"),
    ]
    for (eq, sol) in test_cases:
        assert _equivalent(
            integrate2c(f"x'={eq}", "dt", var_list, use_pade_approx=False), f"x = {sol}"
        )

    # repeat with solutions replaced with (1,1) Pade approximant
    pade_test_cases = [
        ("0", "x"),
        ("a", "x + a*dt"),
        ("a*x", "-x*(a*dt+2)/(a*dt-2)"),
        ("a*x+b", "-(a*dt*x+2*b*dt+2*x)/(a*dt-2)"),
    ]
    for (eq, sol) in pade_test_cases:
        assert _equivalent(
            integrate2c(f"x'={eq}", "dt", var_list, use_pade_approx=True), f"x = {sol}"
        )
