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


def integrate2c(diff_string, t_var, dt_var, vars):
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
    diffeq = sp.Eq(x.diff(t), sp.sympify(diff_string.split("=")[1], locals=sympy_vars))

    # classify ODE, if it is too hard then exit
    ode_properties = set(sp.classify_ode(diffeq))
    if not ode_properties_require_all <= ode_properties:
        raise NotImplementedError("ODE too hard")
    if len(ode_properties_require_one_of & ode_properties) == 0:
        raise NotImplementedError("ODE too hard")

    # try to find analytic solution
    dt = sp.symbols(dt_var, real=True, positive=True)
    x_0 = sp.symbols(dependent_var, real=True)
    solution = sp.dsolve(diffeq, x, ics={x.subs({t: 0}): x_0}).subs({t: dt})
    # note dsolve can return a list of solutions, in which case the above fails

    # return result as C code in NEURON format
    return f"{sp.ccode(x_0)} = {sp.ccode(solution.rhs)}"


def differentiate2c(expression, dependent_var, vars):
    """Analytically differentiate supplied expression, return solution as C code.

    Expression should be of the form "f(x)", where "x" is
    the dependent variable, and the function returns df(x)/dx
    vars should contain the set of all the variables
    referenced by f(x), for example:
    -differentiate2c("a*x", "x", {"a"}) == "a"
    -differentiate2c("cos(y) + b*y**2", "y", {"a","b"}) == "Dy = 2*b*y - sin(y)"

    Args:
        expression: expression to be differentiated e.g. "a*x + b"
        dependent_var: dependent variable, e.g. "x"
        vars: set of all other variables used in expression, e.g. {"a", "b"}

    Returns:
        String containing analytic derivative as C code
    """

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

    # differentiate w.r.t. x
    diff = expr.diff(x)

    # return result as C code in NEURON format
    return sp.ccode(diff)
