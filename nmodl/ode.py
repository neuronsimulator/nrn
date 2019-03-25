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


def _make_unique_prefix(vars, default_prefix="tmp"):
    prefix = default_prefix
    # generate prefix that doesn't match first part
    # of any string in vars
    while True:
        for v in vars:
            # if v is long enough to match prefix
            # and first part of it matches prefix
            if (len(v) >= len(prefix)) and (v[: len(prefix)] == prefix):
                # append undescore to prefix, try again
                prefix += "_"
                break
        else:
            # for loop ended without finding possible clash
            return prefix


def _sympify_eqs(eq_strings, vars, constants):
    # parse eq_strings into sympy expressions
    sympy_vars = {constant: sp.symbols(constant, real=True) for constant in constants}
    state_vars = []
    for var in vars:
        v = sp.symbols(var, real=True)
        sympy_vars[var] = v
        state_vars.append(v)
    eqs = [
        sp.sympify(eq.split("=", 1)[1], locals=sympy_vars)
        - sp.sympify(eq.split("=", 1)[0], locals=sympy_vars)
        for eq in eq_strings
    ]
    return eqs, state_vars, sympy_vars


def solve_lin_system(eq_strings, vars, constants, small_system=False, do_cse=False):
    """Solve linear system of equations, return solution as C code.

    If system is small (small_system=True, typically N<=3):
      - solve analytically by gaussian elimination
      - optionally do Common Subexpression Elimination if do_cse is true

    If system is large (default):
      - gaussian elimination may not be numerically stable at runtime
      - instead return a matrix J and vector F, where J X = F
      - this linear system can then be solved for X by e.g. LU factorization

    Args:
        eqs: list of equations e.g. ["x + y = a", "y = 3 + b"]
        vars: list of variables to solve for, e.g. ["x", "y"]
        constants: set of any other symbolic expressions used, e.g. {"a", "b"}
        small_system: if True, solve analytically by gaussian elimination
                      otherwise return matrix system to be solved
        do_cse: if True, do Common Subexpression Elimination

    Returns:
        List of strings containing assignment statements
        List of strings containing new local variables
    """

    eqs, state_vars, sympy_vars = _sympify_eqs(eq_strings, vars, constants)

    code = []
    new_local_vars = []

    if small_system:
        # small linear system: solve by gaussian elimination
        for rhs in sp.linsolve(eqs, state_vars):
            if do_cse:
                # generate prefix for new local vars that avoids clashes
                prefix = _make_unique_prefix(vars)
                my_symbols = sp.utilities.iterables.numbered_symbols(prefix=prefix)
                sub_exprs, simplified_rhs = sp.cse(
                    rhs, symbols=my_symbols, optimizations="basic", order="canonical"
                )
                for v, e in sub_exprs:
                    new_local_vars.append(sp.ccode(v))
                    code.append(f"{v} = {sp.ccode(e.evalf())}")
                rhs = simplified_rhs[0]
            for v, e in zip(state_vars, rhs):
                code.append(f"{sp.ccode(v)} = {sp.ccode(e.evalf())}")
    else:
        # large linear system: construct and return matrix J, vector F such that
        # J X(t+dt) = F is the Euler linear system to be solved by e.g. LU factorization
        matJ, vecF = sp.linear_eq_to_matrix(eqs, state_vars)
        # construct vector F
        for i, v in enumerate(vecF):
            code.append(f"F[{i}] = {sp.ccode(v.simplify().evalf())}")
        # construct matrix J
        for i, element in enumerate(matJ):
            # todo: fix indexing to be ascending order
            flat_index = matJ.rows * (i % matJ.rows) + (i // matJ.rows)
            code.append(f"J[{flat_index}] = {sp.ccode(element.simplify().evalf())}")

    return code, new_local_vars


def solve_non_lin_system(eq_strings, vars, constants):
    """Solve non-linear system of equations, return solution as C code.

      - returns a vector F, and its Jacobian J, both in terms of X
      - where F(X) = 0 is the implicit equation to solve for X
      - this non-linear system can then be solved with the newton solver

    Args:
        eqs: list of equations e.g. ["x + y = a", "y = 3 + b"]
        vars: list of variables to solve for, e.g. ["x", "y"]
        constants: set of any other symbolic expressions used, e.g. {"a", "b"}

    Returns:
        List of strings containing assignment statements
    """

    eqs, state_vars, sympy_vars = _sympify_eqs(eq_strings, vars, constants)

    jacobian = sp.Matrix(eqs).jacobian(state_vars)

    Xvecsubs = {x_new: sp.symbols(f"X[{i}]") for i, x_new in enumerate(state_vars)}

    code = []
    for i, eq in enumerate(eqs):
        code.append(f"F[{i}] = {sp.ccode(eq.simplify().subs(Xvecsubs).evalf())}")
    for i, jac in enumerate(jacobian):
        # todo: fix indexing to be ascending order
        flat_index = jacobian.rows * (i % jacobian.rows) + (i // jacobian.rows)
        code.append(
            f"J[{flat_index}] = {sp.ccode(jac.simplify().subs(Xvecsubs).evalf())}"
        )

    return code


def integrate2c(diff_string, t_var, dt_var, vars, use_pade_approx=False):
    """Analytically integrate supplied derivative, return solution as C code.

    Derivative should be of the form "x' = f(x)",
    and vars should contain the set of all the variables
    referenced by f(x), for example:

    -``integrate2c ("x' = a*x", "a")``
    -``integrate2c ("x' = a + b*x - sin(3.2)", {"a","b"})``

    Args:
        diff_string: Derivative to be integrated e.g. "x' = a*x"
        t_var: name of time variable in NEURON
        dt_var: name of dt variable in NEURON
        vars: set of variables used in expression, e.g. {"x", "a"}
        use_pade_approx: if False:  return exact solution
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

    - ``nmodl.ode.differentiate2c ("a*x", "x", {"a"}) == "a"``
    - ``differentiate2c ("cos(y) + b*y**2", "y", {"a","b"}) == "Dy = 2*b*y - sin(y)"``

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

    # parse previous expressions in the order that they came in
    # substitute any x-dependent vars in rhs with their rhs expressions,
    # so that the x-dependence (if any) is explicit in each equation
    # and add boolean x_dependent flag for each equation
    prev_eqs = []
    for e in prev_expressions:
        # parse into pairs of (lhs, rhs) SymPy expressions
        e_lhs = sp.sympify(e.split("=", 1)[0], locals=sympy_vars)
        e_rhs = sp.sympify(e.split("=", 1)[1], locals=sympy_vars)
        # substitute in reverse order any x-dependent prior rhs
        for lhs, rhs, x_dependent in reversed(prev_eqs):
            if x_dependent:
                e_rhs = e_rhs.subs(lhs, rhs)
        # differentiate result w.r.t x to check if rhs depends on x
        x_dependent = e_rhs.diff(x).simplify() != 0
        # include boolean x_dependent flag with equation
        prev_eqs.append((e_lhs, e_rhs, x_dependent))

    # want to substitute equations in reverse order
    prev_eqs.reverse()

    # substitute each x-dependent prev equation in reverse order
    for lhs, rhs, x_dependent in prev_eqs:
        if x_dependent:
            expr = expr.subs(lhs, rhs)

    # differentiate w.r.t. x
    diff = expr.diff(x).simplify()

    # try to simplify expression in terms of existing variables
    # ignore any exceptions here, since we already have a valid solution
    # so if this further simplification step fails the error is not fatal
    try:
        # if expression is equal to one of the supplied vars, replace with this var
        # can do a simple string comparison here since a var cannot be further simplified
        diff_as_string = sp.ccode(diff)
        for v in sympy_vars:
            if diff_as_string == sp.ccode(sympy_vars[v]):
                diff = sympy_vars[v]

        # or if equal to rhs of one of the supplied equations, replace with lhs
        # we need to substitute the constant prev expressions on both sides before comparing
        for i_eq, prev_eq in enumerate(prev_eqs):
            # each supplied eq, as well as our solution diff, need
            # recursive substitution of any preceeding non-x-dependent statements
            # before comparison
            eq_rhs_sub = prev_eq[1]
            diff_sub = diff
            for lhs, rhs, x_dependent in prev_eqs[i_eq:]:
                if not x_dependent:
                    eq_rhs_sub = eq_rhs_sub.subs(lhs, rhs)
                    diff_sub = diff_sub.subs(lhs, rhs)
            if (diff_sub - eq_rhs_sub).simplify() == 0:
                diff = prev_eq[0]
    except Exception:
        pass

    # return result as C code in NEURON format
    return sp.ccode(diff.evalf())


def forwards_euler2c(diff_string, dt_var, vars):
    """Return forwards euler solution of diff_string as C code.

    Derivative should be of the form "x' = f(x)",
    and vars should contain the set of all the variables
    referenced by f(x), for example:

    - ``forwards_euler2c("x' = a*x", "a")``
    - ``forwards_euler2c("x' = a + b*x - sin(3.2)", {"a","b"})``

    Args:
        diff_string: Derivative to be integrated e.g. "x' = a*x"
        dt_var: name of dt variable in NEURON
        vars: set of variables used in expression, e.g. {"x", "a"}

    Returns:
        String containing forwards Euler timestep as C code
    """

    # every symbol (a.k.a variable) that SymPy
    # is going to manipulate needs to be declared
    # explicitly
    sympy_vars = {}
    vars = set(vars)
    dependent_var = diff_string.split("=")[0].split("'")[0].strip()
    x = sp.symbols(dependent_var, real=True)
    vars.discard(dependent_var)
    # declare all other supplied variables
    sympy_vars = {var: sp.symbols(var, real=True) for var in vars}
    sympy_vars[dependent_var] = x

    # parse string into SymPy equation
    diffeq_rhs = sp.sympify(diff_string.split("=", 1)[1], locals=sympy_vars)

    # forwards Euler solution is x + dx/dt * dt
    dt = sp.symbols(dt_var, real=True, positive=True)
    solution = (x + diffeq_rhs * dt).simplify().evalf()

    # return result as C code in NEURON format
    return f"{sp.ccode(x)} = {sp.ccode(solution)}"
