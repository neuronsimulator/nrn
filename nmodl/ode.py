# ***********************************************************************
# Copyright (C) 2018-2019 Blue Brain Project
#
# This file is part of NMODL distributed under the terms of the GNU
# Lesser General Public License. See top-level LICENSE file for details.
# ***********************************************************************

import sympy as sp

major, minor = (int(v) for v in sp.__version__.split(".")[:2])
if not ((major >= 1) and (minor >= 2)):
    raise ImportError(f"Requires SympPy version >= 1.2, found {major}.{minor}")


def integrate2c(diff_string, t_var, dt_var, vars, use_pade_approx=False):
    """Analytically integrate supplied derivative, return solution as C code.

    Derivative should be of the form "x' = f(x)",
    and vars should contain the set of all the variables
    referenced by f(x), for example:
    -integrate2c("x' = a*x", "a")
    -integrate2c("x' = a + b*x - sin(3.2)", {"a","b"})

    Args:
        diff_string: Derivative to be integrated e.g. "x' = a*x"
        t_var: name of time variable in NEURON
        dt_var: name of dt variable in NEURON
        vars: set of variables used in expression, e.g. {"x", "a"}
        ues_pade_approx][]: if False:  return exact solution
                         if True:   return (1,1) Pade approx to solution
                                    correct to second order in dt_var

    Returns:
        String containing analytic integral of derivative as C code

    Raises:
        NotImplementedError: if ODE is too hard, or if fails to solve it.
        ImportError: if SymPy version is too old (<1.2)
    """

    # only try to solve ODEs that are not too hard
    ode_properties_require_all = {"separable"}
    ode_properties_require_one_of = {
        "1st_exact",
        "1st_linear",
        "almost_linear",
        "nth_linear_constant_coeff_homogeneous",
        "1st_exact_Integral",
        "1st_linear_Integral",
    }

    # every symbol (a.k.a variable) that SymPy
    # is going to manipulate needs to be declared
    # explicitly
    sympy_vars = {}
    t = sp.symbols(t_var, real=True, positive=True)
    vars = set(vars)
    vars.discard(t_var)
    # the dependent variable is a function of t
    # we use the python variable name x for this
    dependent_var = diff_string.split("=")[0].split("'")[0].strip()
    x = sp.Function(dependent_var, real=True)(t)
    vars.discard(dependent_var)
    # declare all other supplied variables
    sympy_vars = {var: sp.symbols(var) for var in vars}
    sympy_vars[dependent_var] = x
    sympy_vars[t_var] = t

    # parse string into SymPy equation
    diffeq = sp.Eq(
        x.diff(t), sp.sympify(diff_string.split("=", 1)[1], locals=sympy_vars)
    )

    # classify ODE, if it is too hard then exit
    ode_properties = set(sp.classify_ode(diffeq))
    if not ode_properties_require_all <= ode_properties:
        raise NotImplementedError("ODE too hard")
    if len(ode_properties_require_one_of & ode_properties) == 0:
        raise NotImplementedError("ODE too hard")

    # try to find analytic solution
    dt = sp.symbols(dt_var, real=True, positive=True)
    x_0 = sp.symbols(dependent_var, real=True)
    # note dsolve can return a list of solutions, in which case this fails:
    solution = sp.dsolve(diffeq, x, ics={x.subs({t: 0}): x_0}).subs({t: dt}).rhs

    if use_pade_approx:
        # (1,1) order pade approximant, correct to 2nd order in dt,
        # constructed from coefficients of 2nd order taylor expansion
        taylor_series = sp.Poly(sp.series(solution, dt, 0, 3).removeO(), dt)
        _a0 = taylor_series.nth(0)
        _a1 = taylor_series.nth(1)
        _a2 = taylor_series.nth(2)
        solution = (
            (_a0 * _a1 + (_a1 * _a1 - _a0 * _a2) * dt) / (_a1 - _a2 * dt)
        ).simplify()

    # return result as C code in NEURON format
    return f"{sp.ccode(x_0)} = {sp.ccode(solution)}"


def differentiate2c(expression, dependent_var, vars, prev_expressions=None):
    """Analytically differentiate supplied expression, return solution as C code.

    Expression should be of the form "f(x)", where "x" is
    the dependent variable, and the function returns df(x)/dx

    The set vars must contain all variables used in the expression.

    Furthermore, if any of these variables are themselves functions that should
    be substituted before differentiating, they can be supplied in the prev_expressions list.
    Before differentiating each of these expressions will be substituted into expressions,
    where possible, in reverse order - i.e. starting from the end of the list.

    If the result coincides with one of the vars, or the LHS of one of
    the prev_expressions, then it is simplified to this expression.

    Some simple examples of use:
    -differentiate2c("a*x", "x", {"a"}) == "a"
    -differentiate2c("cos(y) + b*y**2", "y", {"a","b"}) == "Dy = 2*b*y - sin(y)"

    Args:
        expression: expression to be differentiated e.g. "a*x + b"
        dependent_var: dependent variable, e.g. "x"
        vars: set of all other variables used in expression, e.g. {"a", "b", "c"}
        prev_expressions: time-ordered list of preceeding expressions
                          to evaluate & substitute, e.g. ["b = x + c", "a = 12*b"]

    Returns:
        String containing analytic derivative of expression (including any substitutions
        of variables from supplied prev_expressions) w.r.t dependent_var as C code.
    """
    prev_expressions = prev_expressions or []
    # every symbol (a.k.a variable) that SymPy
    # is going to manipulate needs to be declared
    # explicitly
    x = sp.symbols(dependent_var, real=True)
    vars = set(vars)
    vars.discard(dependent_var)
    # declare all other supplied variables
    sympy_vars = {var: sp.symbols(var, real=True) for var in vars}
    sympy_vars[dependent_var] = x

    # parse string into SymPy equation
    expr = sp.sympify(expression, locals=sympy_vars)

    # parse previous equations into (lhs, rhs) pairs & reverse order
    prev_eqs = [
        (
            sp.sympify(e.split("=", 1)[0], locals=sympy_vars),
            sp.sympify(e.split("=", 1)[1], locals=sympy_vars),
        )
        for e in prev_expressions
    ]
    prev_eqs.reverse()

    # substitute each prev equation in reverse order: latest first
    for eq in prev_eqs:
        expr = expr.subs(eq[0], eq[1])

    # differentiate w.r.t. x
    diff = expr.diff(x).simplify()

    # if expression is equal to one of the supplied vars, replace with this var
    for v in sympy_vars:
        if (diff - sympy_vars[v]).simplify() == 0:
            diff = sympy_vars[v]
    # or if equal to rhs of one of supplied equations, replace with lhs
    for i_eq, eq in enumerate(prev_eqs):
        # each supplied eq also needs recursive substitution of preceeding statements
        # here, before comparison with diff expression
        expr = eq[1]
        for sub_eq in prev_eqs[i_eq:]:
            expr = expr.subs(sub_eq[0], sub_eq[1])
        if (diff - expr).simplify() == 0:
            diff = eq[0]

    # return result as C code in NEURON format
    return sp.ccode(diff)
