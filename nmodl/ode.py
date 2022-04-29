# ***********************************************************************
# Copyright (C) 2018-2022 Blue Brain Project
#
# This file is part of NMODL distributed under the terms of the GNU
# Lesser General Public License. See top-level LICENSE file for details.
# ***********************************************************************

from importlib import import_module

import sympy as sp

# import known_functions through low-level mechanism because the ccode
# module is overwritten in sympy and contents of that submodule cannot be
# accessed through regular imports
major, minor = (int(v) for v in sp.__version__.split(".")[:2])
if major >= 1 and minor >= 7:
    known_functions = import_module('sympy.printing.c').known_functions_C99
else:
    known_functions = import_module('sympy.printing.ccode').known_functions_C99
known_functions.pop('Abs')
known_functions['abs'] = 'fabs'


if not ((major >= 1) and (minor >= 2)):
    raise ImportError(f"Requires SympPy version >= 1.2, found {major}.{minor}")

def _get_custom_functions(fcts):
    custom_functions = {}
    for f in fcts:
        if not f in known_functions.keys():
            custom_functions[f] = f
    return custom_functions


def _var_to_sympy(var_str):
    """Return sympy variable from string representing variable

    If string contains "[" it is assumed to be an array variable
    of the form "variable_name[N]", where N is the size of the array

    Args:
        var_str: variable as string

    Returns:
        (variable_name_as_string, variable_as_sympy_object)
    """
    if "[" in var_str:
        # var is an array variable with defined size, e.g. X[5]
        var_name = var_str.split("[", 1)[0].strip()
        var_len = int(var_str.split("[", 1)[1].split("]", 1)[0].strip())
        # SymPy equivalent of an array is IndexedBase:
        return var_name, sp.IndexedBase(var_name, shape=(var_len,), real=True)
    else:
        # otherwise can use a standard SymPy symbol:
        return var_str, sp.symbols(var_str, real=True)


def _sympify_diff_eq(diff_string, vars):
    """Parse differential equation into sympy expression

    Given eq_string of the form "x' = df/dx", return sympy
    objects representing x and df/dx

    If x is an array, then it should be declared in vars
    as "x[N]", where N is the size of the array, and an
    IndexedBase sympy object will be returned

    Args:
        eq_string: string containing differential equation
        vars: list of strings containing vars used in equation

    Returns:
        x: sympy object representing x
        dxdt: sympy expression representing df/dx
    """
    sympy_vars = {}
    for var in vars:
        var_name, sympy_object = _var_to_sympy(var)
        sympy_vars[var_name] = sympy_object

    diff_string_lhs, diff_string_rhs = diff_string.split("=", 1)

    # parse dependent variable from LHS of equation:
    x = sp.sympify(diff_string_lhs.replace("'", ""), locals=sympy_vars)

    # parse RHS of equation into SymPy expression
    dxdt = sp.sympify(diff_string_rhs, locals=sympy_vars)

    return x, dxdt


def _sympify_eqs(eq_strings, state_vars, vars):
    """Parse equations into sympy expressions

    Given lists of strings containing equations, state variables,
    and constants, it parses the equations into sympy expressions.

    Args:
        eq_strings: list of strings containing equations
        state_vars: list of strings containing state vars,
                    if array then index must be specifiec
        vars: list of strings containing constant vars
              if array then size of array should be specified, e.g. X[10]

    Returns:
        eqs: list of sympy expressions
        state_vars: list of sympy objects for vars
        sympy_vars: dict of name:sympy_object for all vars & constants
    """

    # convert all vars into sympy objects
    sympy_vars = {}
    for var in vars:
        var_name, sympy_object = _var_to_sympy(var)
        sympy_vars[var_name] = sympy_object

    # parse state vars & eqs using above sympy objects
    sympy_state_vars = []
    for state_var in state_vars:
        sympy_state_vars.append(sp.sympify(state_var, locals=sympy_vars))
    eqs = [
        (sp.sympify(eq.split("=", 1)[1], locals=sympy_vars)
        - sp.sympify(eq.split("=", 1)[0], locals=sympy_vars)).expand()
        for eq in eq_strings
    ]
    return eqs, sympy_state_vars, sympy_vars

def _interweave_eqs(F, J):
    """Interweave F and J equations so that they are printed in code
    rowwise from the equation J x = F. For example:

    F = [F_0,
         F_1,
         F_2]

    (Jmat is not the actual J in the argument, it is here to the sake of
    clarity)
    Jmat = [J_0, J_3, J_6
            J_1, J_4, J_7
            J_2, J_5, J_8]
    (J is the actual input with the following ordering)
    J = [J_0,
         J_3,
         J_6,
         J_1,
         J_4,
         J_7,
         J_2,
         J_5,
         J_8]

    What we want is:
    code = [F_0,
            J_0,
            J_3,
            J_6,
            F_1,
            J_1,
            J_4,
            J_7,
            F_2,
            J_2,
            J_5,
            J_8]

    Args:
        F: F vector
        J: J matrix represented as a vector (rowwise)

    Returns:
        code: F and J interweaved in one vector
    """
    code = []
    n = len(F)
    for i, expr in enumerate(F):
        code.append(expr)
        for j in range(i * n, (i+1) * n):
            code.append(J[j])

    return code


def solve_lin_system(eq_strings, vars, constants, function_calls, tmp_unique_prefix, small_system=False, do_cse=False):
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
        function_calls: set of function calls used in the ODE
        tmp_unique_prefix: is a unique prefix on which new variables can be easily created
                       by appending strings. It is usually of the form "tmp"
        small_system: if True, solve analytically by gaussian elimination
                      otherwise return matrix system to be solved
        do_cse: if True, do Common Subexpression Elimination

    Returns:
        code: list of strings containing assignment statements
        vars: list of strings containing new local variables
    """

    eqs, state_vars, sympy_vars = _sympify_eqs(eq_strings, vars, constants)
    custom_fcts = _get_custom_functions(function_calls)

    code = []
    new_local_vars = []

    if small_system:
        # small linear system: solve by gaussian elimination
        solution_vector = sp.linsolve(eqs, state_vars).args[0]
        if do_cse:
            # generate prefix for new local vars that avoids clashes
            my_symbols = sp.utilities.iterables.numbered_symbols(prefix=tmp_unique_prefix + '_')
            sub_exprs, simplified_solution_vector = sp.cse(
                solution_vector,
                symbols=my_symbols,
                optimizations="basic",
                order="canonical",
            )
            for var, expr in sub_exprs:
                new_local_vars.append(sp.ccode(var))
                code.append(f"{var} = {sp.ccode(expr.evalf())}")
            solution_vector = simplified_solution_vector[0]
        for var, expr in zip(state_vars, solution_vector):
            code.append(f"{sp.ccode(var)} = {sp.ccode(expr.evalf(), contract=False, user_functions=custom_fcts)}")
    else:
        # large linear system: construct and return matrix J, vector F such that
        # J X = F is the linear system to be solved for X by e.g. LU factorization
        matJ, vecF = sp.linear_eq_to_matrix(eqs, state_vars)

        # construct vector F
        vecFcode = []
        for i, expr in enumerate(vecF):
            vecFcode.append(f"F[{i}] = {sp.ccode(expr.simplify().evalf())}")
        # construct matrix J
        vecJcode = []
        for i, expr in enumerate(matJ):
            # todo: fix indexing to be ascending order
            flat_index = matJ.rows * (i % matJ.rows) + (i // matJ.rows)
            vecJcode.append(f"J[{flat_index}] = {sp.ccode(expr.simplify().evalf(), user_functions=custom_fcts)}")
        # interweave
        code = _interweave_eqs(vecFcode, vecJcode)

    return code, new_local_vars


def solve_non_lin_system(eq_strings, vars, constants, function_calls):
    """Solve non-linear system of equations, return solution as C code.

      - returns a vector F, and its Jacobian J, both in terms of X
      - where F(X) = 0 is the implicit equation to solve for X
      - this non-linear system can then be solved with the newton solver

    Args:
        eqs: list of equations e.g. ["x + y = a", "y = 3 + b"]
        vars: list of variables to solve for, e.g. ["x", "y"]
        constants: set of any other symbolic expressions used, e.g. {"a", "b"}
        function_calls: set of function calls used in the ODE

    Returns:
        List of strings containing assignment statements
    """

    eqs, state_vars, sympy_vars = _sympify_eqs(eq_strings, vars, constants)

    custom_fcts = _get_custom_functions(function_calls)

    jacobian = sp.Matrix(eqs).jacobian(state_vars)

    X_vec_map = {x: sp.symbols(f"X[{i}]") for i, x in enumerate(state_vars)}

    vecFcode = []
    for i, eq in enumerate(eqs):
        vecFcode.append(f"F[{i}] = {sp.ccode(eq.simplify().subs(X_vec_map).evalf())}")
    vecJcode = []
    for i, jac in enumerate(jacobian):
        # todo: fix indexing to be ascending order
        flat_index = jacobian.rows * (i % jacobian.rows) + (i // jacobian.rows)
        vecJcode.append(
            f"J[{flat_index}] = {sp.ccode(jac.simplify().subs(X_vec_map).evalf(), user_functions=custom_fcts)}"
        )
    # interweave
    code = _interweave_eqs(vecFcode, vecJcode)

    return code


def integrate2c(diff_string, dt_var, vars, use_pade_approx=False):
    """Analytically integrate supplied derivative, return solution as C code.

    Given a differential equation of the form x' = f(x), the value of
    x at time t+dt is found in terms of the value of x at time t:
    x(t + dt) = g( x(t), dt )
    and this equation is returned in the format NEURON expects:
    x = g( x, dt ),
    where the x on the right is the current value of x at time t,
    and the x on the left is the new value of x at time t+dt

    The derivative should be of the form "x' = f(x)",
    and vars should contain the set of all the variables
    referenced by f(x), for example:

    -``integrate2c("x' = a*x", "dt", {"a"})``
    -``integrate2c("x' = a + b*x - sin(3.2)", "dt", {"a","b"})``

    Optionally, the analytic result can be expanded in powers of dt,
    and the (1,1) Pade approximant to the solution returned.
    This approximate solution is correct to second order in dt.

    Args:
        diff_string: Derivative to be integrated e.g. "x' = a*x + b"
        t_var: name of time variable t in NEURON
        dt_var: name of timestep variable dt in NEURON
        vars: set of variables used in expression, e.g. {"a", "b"}
        use_pade_approx: if False, return exact solution
                         if True, return (1,1) Pade approx to solution
                         correct to second order in dt_var

    Returns:
        string containing analytic integral of derivative as C code
    Raises:
        NotImplementedError: if the ODE is too hard, or if it fails to solve it.
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

    x, dxdt = _sympify_diff_eq(diff_string, vars)
    # set up differential equation d(x(t))/dt = ...
    # where the function x_t = x(t) is substituted for the symbol x
    # the dependent variable is a function of t
    t = sp.Dummy("t", real=True, positive=True)
    x_t = sp.Function("x(t)", real=True)(t)
    diffeq = sp.Eq(x_t.diff(t), dxdt.subs({x: x_t}))

    # for simple linear case write down solution in preferred form:
    dt = sp.symbols(dt_var, real=True, positive=True)
    solution = None
    c1 = dxdt.diff(x).simplify()
    if c1 == 0:
        # constant equation:
        # x' = c0
        # x(t+dt) = x(t) + c0 * dt
        solution = (x + dt * dxdt).simplify()
    elif c1.diff(x) == 0:
        # linear equation:
        # x' = c0 + c1*x
        # x(t+dt) = (-c0 + (c0 + c1*x(t))*exp(c1*dt))/c1
        c0 = (dxdt - c1 * x).simplify()
        solution = (-c0 / c1).simplify() + (c0 + c1 * x).simplify() * sp.exp(
            c1 * dt
        ) / c1
    else:
        # otherwise try to solve ODE with sympy:
        # first classify ODE, if it is too hard then exit
        ode_properties = set(sp.classify_ode(diffeq))
        if not ode_properties_require_all <= ode_properties:
            raise NotImplementedError("ODE too hard")
        if len(ode_properties_require_one_of & ode_properties) == 0:
            raise NotImplementedError("ODE too hard")
        # try to find analytic solution, with initial condition x_t(t=0) = x
        # (note dsolve can return a list of solutions, in which case this currently fails)
        solution = sp.dsolve(diffeq, x_t, ics={x_t.subs({t: 0}): x})
        # evaluate solution at x(dt), extract rhs of expression
        solution = solution.subs({t: dt}).rhs.simplify()

    if use_pade_approx:
        # (1,1) order Pade approximant, correct to 2nd order in dt,
        # constructed from the coefficients of 2nd order Taylor expansion
        taylor_series = sp.Poly(sp.series(solution, dt, 0, 3).removeO(), dt)
        _a0 = taylor_series.nth(0)
        _a1 = taylor_series.nth(1)
        _a2 = taylor_series.nth(2)
        solution = (
            (_a0 * _a1 + (_a1 * _a1 - _a0 * _a2) * dt) / (_a1 - _a2 * dt)
        ).simplify()
        # special case where above form gives 0/0 = NaN
        if _a1 == 0 and _a2 == 0:
            solution = _a0

    # return result as C code in NEURON format:
    #   - in the lhs x_0 refers to the state var at time (t+dt)
    #   - in the rhs x_0 refers to the state var at time t
    return f"{sp.ccode(x)} = {sp.ccode(solution.evalf())}"


def forwards_euler2c(diff_string, dt_var, vars, function_calls):
    """Return forwards euler solution of diff_string as C code.

    Derivative should be of the form "x' = f(x)",
    and vars should contain the set of all the variables
    referenced by f(x), for example:
    -forwards_euler2c("x' = a*x", ["a","x"])
    -forwards_euler2c("x' = a + b*x - sin(3.2)", {"x","a","b"})

    Args:
        diff_string: Derivative to be integrated e.g. "x' = a*x"
        dt_var: name of timestep dt variable in NEURON
        vars: set of variables used in expression, e.g. {"x", "a"}
        function_calls: set of function calls used in the ODE

    Returns:
        String containing forwards Euler timestep as C code
    """
    x, dxdt = _sympify_diff_eq(diff_string, vars)
    # forwards Euler solution is x + dx/dt * dt
    dt = sp.symbols(dt_var, real=True, positive=True)
    solution = (x + dxdt * dt).simplify().evalf()

    custom_fcts = _get_custom_functions(function_calls)
    # return result as C code in NEURON format
    return f"{sp.ccode(x)} = {sp.ccode(solution, user_functions=custom_fcts)}"


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
        string containing analytic derivative of expression (including any substitutions
        of variables from supplied prev_expressions) w.r.t. dependent_var as C code.
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
