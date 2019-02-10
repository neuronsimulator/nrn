import sympy as sp

def integrate2c(diff_string, vars):
    """Analytically integrate supplied derivative, return solution as C code.

    Derivative should be of the form "x' = f(x)",
    and vars should contain the set of all the variables
    referenced by f(x), for example:
    -integrate2c("x' = a*x", "a")
    -integrate2c("x' = a + b*x - sin(3.2)", {"a","b"})

    Args:
        diff_string: Derivative to be integrated e.g. "x' = a*x"
        vars: set of variables used in expression, e.g. {"x", "a"}

    Returns:
        String containing analytic integral of derivative as C code

    Raises:
        NotImplementedError: if ODE is too hard, or if fails to solve it.
    """

    # only try to solve ODEs that are not too hard
    ode_properties_require_all = {'separable'}
    ode_properties_require_one_of = {
        '1st_exact',
        '1st_linear',
        'almost_linear',
        'nth_linear_constant_coeff_homogeneous',
        '1st_exact_Integral',
        '1st_linear_Integral',
    }

    # every symbol (a.k.a variable) that SymPy
    # is going to manipulate needs to be declared
    # explicitly
    sympy_vars = {}
    # time is always t
    t = sp.symbols('t', real=True, positive=True)
    vars = set(vars)
    vars.discard("t")
    # the dependent variable is a function of t
    # we use the python variable name x for this
    dependent_var = diff_string.split('=')[0].split("'")[0].strip()
    x = sp.Function(dependent_var, real=True)(t)
    vars.discard(dependent_var)
    # declare all other supplied variables
    sympy_vars = {var: sp.symbols(var) for var in vars}
    sympy_vars[dependent_var] = x
    sympy_vars["t"] = t

    # parse string into SymPy equation
    diffeq = sp.Eq(
        x.diff(t),
        sp.sympify(
            diff_string.split('=')[1],
            locals=sympy_vars))

    # classify ODE, if it is too hard then exit
    ode_properties = set(sp.classify_ode(diffeq))
    if not ode_properties_require_all <= ode_properties:
        raise NotImplementedError("ODE too hard")
    if len(ode_properties_require_one_of & ode_properties) == 0:
        raise NotImplementedError("ODE too hard")

    # try to find analytic solution
    dt = sp.symbols('dt', real=True, positive=True)
    x_0 = sp.symbols(dependent_var, real=True)
    solution = sp.dsolve(diffeq, x, ics={x.subs({t: 0}): x_0}).subs({t: dt})
    # note dsolve can return a list of solutions, in which case the above fails

    # return result as C code in NEURON format
    return f"{sp.ccode(x_0)} = {sp.ccode(solution.rhs)}"
