# Copyright 2023 Blue Brain Project, EPFL.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: Apache-2.0

from neuron.nmodl.ode import (
    differentiate2c,
    integrate2c,
    make_symbol,
    mangle_protected_identifiers,
    demangle_protected_identifiers,
    transform_expression,
    discretize_derivative,
    solve_non_lin_system,
)
import pytest
import cmath
import re

import sympy as sp


def test_mangle():
    # These inputs should be mangled
    test_cases = [
        # python keywords
        [
            "and",
            "break",
            "class",
            "continue",
            "def",
            "del",
            "from",
            "in",
            "is",
            "lambda",
            "pass",
            "yield",
        ],
        # sympy keywords
        "Symbol",
        [
            "beta",
            "gamma",
            "uppergamma",
            "lowergamma",
            "polygamma",
            "loggamma",
            "digamma",
            "trigamma",
            "gamma",
        ],
    ]
    for eqs in test_cases:
        mangled = mangle_protected_identifiers(eqs)
        assert eqs != mangled  # name-mangling was applied
        if isinstance(eqs, list):
            # check that name-mangling was applied to each element in the list
            assert all(a != b for a, b in zip(eqs, mangled))
        sp.sympify(mangled)
        demangled = demangle_protected_identifiers(mangled)
        assert demangled == eqs  # restored to original state
    # These inputs should NOT be mangled
    test_cases = [
        # No conflict
        [],
        "",
        "foobar",
        # Built-in functions for both NMODL and SymPy
        ["exp", "log", "atan", "erf", "sin", "sinh"],
        ["atan2(3,4)", "x", "y", "z"],
    ]
    for eqs in test_cases:
        mangled = mangle_protected_identifiers(eqs)
        assert eqs == mangled  # no change
        if mangled:
            sp.sympify(mangled)
        demangled = demangle_protected_identifiers(mangled)
        assert demangled == eqs  # no change


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
    lhs = mangle_protected_identifiers(lhs)
    rhs = mangle_protected_identifiers(rhs)
    lhs = lhs.replace("pow(", "Pow(")
    rhs = rhs.replace("pow(", "Pow(")
    sympy_vars = {str(var): make_symbol(var) for var in vars}
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

    # SymPy can manipulate transcendental nmodl functions ("sin" & "cos" are not mangled)
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
        differentiate2c("a*x + b", "x", {"a", "b"}, ["b = 2*x", "a = -2*x*x"]),
        "-6*x*x+2",
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

    assert _equivalent(
        differentiate2c(
            "(s[0] + s[1])*(z[0]*z[1]*z[2])*x",
            "x",
            {sp.IndexedBase("s", shape=[1]), sp.IndexedBase("z", shape=[1])},
        ),
        "(s[0] + s[1])*(z[0]*z[1]*z[2])",
        {sp.IndexedBase("s", shape=[1]), sp.IndexedBase("z", shape=[1])},
    )

    # make sure we can diff against indexed vars as well
    var = sp.IndexedBase("x", shape=[1])

    assert _equivalent(
        differentiate2c(
            "a * x[0]",
            var[0],
            {"a", var},
        ),
        "a",
        {"a"},
    )

    result = differentiate2c(
        "-f(x)",
        "x",
        {},
    )
    # instead of comparing the expression as a string, we convert the string
    # back to an expression and compare with an explicit function
    size = 100
    for index in range(size):
        a, b = -5, 5
        value = (b - a) * index / size + a
        pytest.approx(
            float(
                sp.sympify(result)
                .subs(sp.Function("f"), sp.sin)
                .subs({"x": value})
                .evalf()
            )
        ) == float(
            -sp.Derivative(sp.sin("x"))
            .as_finite_difference(1e-3)
            .subs({"x": value})
            .evalf()
        )
    with pytest.raises(ValueError):
        differentiate2c(
            "-f(x)",
            "x",
            {},
            stepsize=-1,
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
        # Check that sympy can manipulate nmodl's transcendental functions
        ("-exp(x)", "x - log(dt * exp(x) + 1)"),
        # assume custom_function is defined in mod file
        ("custom_function(a)*x", "x*exp(custom_function(a)*dt)"),
        # name clashes with sympy
        ("beta(a)*x", "x*exp(beta(a)*dt)"),
    ]
    for eq, sol in test_cases:
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
    for eq, sol in pade_test_cases:
        assert _equivalent(
            integrate2c(f"x'={eq}", "dt", var_list, use_pade_approx=True), f"x = {sol}"
        )


def test_finite_difference():
    df_dx = "(f(x + x_delta_/2) - f(x - x_delta_/2))/x_delta_"
    dg_dx = "(g(x + x_delta_/2) - g(x - x_delta_/2))/x_delta_"

    test_cases = [
        ("f(x)", df_dx),
        ("a*f(x)", f"a*{df_dx}"),
        ("a*f(x)*g(x)", f"a*({df_dx}*g(x) + f(x)*{dg_dx})"),
        ("a*f(x) + b*g(x)", f"a*{df_dx} + b*{dg_dx}"),
    ]
    vars = ["a", "x", "x_delta_"]

    for expr, expected in test_cases:
        expr = sp.diff(sp.sympify(expr), "x")
        actual = transform_expression(expr, discretize_derivative)
        msg = f"'{actual}'  =!=  '{expected}'"

        assert _equivalent(str(actual), expected, vars=vars), msg


@pytest.mark.parametrize(
    "rhs",
    [
        ["-u + f(u)"],
        ["-u + f(a*u + b)"],
        ["-u + f(u*u + b)"],
        ["-u + f(g(a*u + b))"],
        ["-u + u*f(a*u + b)"],
        ["-u + f(a*u + b)*g(u)"],
        ["-u + h(a*u, u + b)"],
        ["-u + f(a*u - b*v)", "(-v + f(b*u - a*v))/tau"],
        ["-u + h(u*v, u + v)", "-v + f(g(u - v))"],
    ],
)
def test_non_linear_composite_function_jacobian(rhs):
    """Finite differences must use a bound step for the original state."""
    states = ["u", "v"][: len(rhs)]
    equations = [f"({x} - old_{x})/dt = {r}" for x, r in zip(states, rhs)]
    parameters = dict(a=-1.7, b=0.4, tau=1.3, dt=0.025, old_u=0.2, old_v=-0.1)
    functions = {
        "f": lambda z: cmath.sin(z) + 0.1 * z**3,
        "g": cmath.cos,
        "h": lambda z, w: cmath.sin(z) + z * w * w,
    }
    code = solve_non_lin_system(equations, states, set(parameters), set(functions))
    assert not re.search(r"(?<![A-Za-z0-9_])_[A-Za-z0-9_]+", "\n".join(code))
    for statement in code:
        if statement.startswith("J["):
            flat_index = int(statement.split("]")[0][2:])
            steps = set(re.findall(r"dX_\[(\d+)\]", statement))
            assert steps <= {str(flat_index // len(states))}

    # A complex-step derivative of the original residual is independent of
    # both SymPy differentiation and the generated real finite differences.
    for values in [[0.0, 0.0], [0.31, -0.27], [-1.1, 0.7]]:
        values = values[: len(states)]
        environment = {
            **parameters,
            **functions,
            "X": values,
            "dX_": [1e-5, 3e-5][: len(states)],
            "F": [0.0] * len(states),
            "J": [0.0] * len(states) ** 2,
            "pow": pow,
        }
        for statement in code:
            exec(statement, {"__builtins__": {}}, environment)
        for i, expression in enumerate(rhs):
            for j, state in enumerate(states):
                shifted = dict(zip(states, values))
                shifted[state] += 1e-30j
                residual = eval(
                    f"({expression}) - ({states[i]} - old_{states[i]})/dt",
                    {"__builtins__": {}},
                    {**parameters, **functions, **shifted},
                )
                assert environment["J"][i + len(states) * j] == pytest.approx(
                    residual.imag / 1e-30, rel=2e-7, abs=2e-8
                )


def test_non_linear_reported_two_state_system():
    equations = [
        "(uu - old_uu)/dt = -uu + f(aee*uu - aie*vv - ze + i_e)",
        "(vv - old_vv)/dt = (-vv + f(aei*uu - aii*vv - zi + i_i))/tau",
    ]
    parameters = dict(
        aee=1.7,
        aie=0.4,
        aei=-0.6,
        aii=1.2,
        ze=0.3,
        zi=-0.2,
        i_e=0.7,
        i_i=-0.4,
        tau=1.3,
        dt=0.025,
        old_uu=0.2,
        old_vv=-0.1,
    )
    code = solve_non_lin_system(equations, ["uu", "vv"], set(parameters), {"f"})
    environment = {
        **parameters,
        "f": cmath.sin,
        "X": [0.31, -0.27],
        "dX_": [1e-5, 3e-5],
        "F": [0.0] * 2,
        "J": [0.0] * 4,
    }
    for statement in code:
        exec(statement, {"__builtins__": {}}, environment)
    z_e = (
        parameters["aee"] * 0.31
        - parameters["aie"] * (-0.27)
        - parameters["ze"]
        + parameters["i_e"]
    )
    z_i = (
        parameters["aei"] * 0.31
        - parameters["aii"] * (-0.27)
        - parameters["zi"]
        + parameters["i_i"]
    )
    expected = [
        -1 - 1 / parameters["dt"] + parameters["aee"] * cmath.cos(z_e),
        parameters["aei"] * cmath.cos(z_i) / parameters["tau"],
        -parameters["aie"] * cmath.cos(z_e),
        (-1 - parameters["aii"] * cmath.cos(z_i)) / parameters["tau"]
        - 1 / parameters["dt"],
    ]
    assert environment["J"] == pytest.approx(expected, rel=2e-7, abs=2e-8)


def test_non_linear_composite_preserves_exact_terms():
    code = solve_non_lin_system(["0 = a*u + f(b*u) + c"], ["u"], {"a", "b", "c"}, {"f"})
    jacobian = next(line.split("=", 1)[1] for line in code if line.startswith("J["))
    x, dx, a, b, c = sp.symbols("x dx a b c")
    actual = sp.sympify(
        jacobian, locals={"X": [x], "dX_": [dx], "a": a, "b": b, "c": c}
    )
    f = sp.Function("f")
    expected = a + (f(b * (x + dx / 2)) - f(b * (x - dx / 2))) / dx
    assert sp.simplify(sp.nsimplify(actual) - expected) == 0
    assert c not in actual.free_symbols


def test_non_linear_composite_indexed_state():
    code = solve_non_lin_system(
        ["0 = -u[0] + f(a*u[0] + b)"], ["u[0]"], {"u[1]", "a", "b"}, {"f"}
    )
    assert not any("_xi_" in line or "_delta_" in line for line in code)
    assert any("dX_[0]" in line for line in code)
    environment = {
        "X": [0.31],
        "dX_": [1e-5],
        "F": [0.0],
        "J": [0.0],
        "a": -1.7,
        "b": 0.4,
        "f": cmath.sin,
    }
    for statement in code:
        exec(statement, {"__builtins__": {}}, environment)
    assert environment["J"][0] == pytest.approx(
        -1 - 1.7 * cmath.cos(-1.7 * 0.31 + 0.4), rel=2e-7, abs=2e-8
    )
