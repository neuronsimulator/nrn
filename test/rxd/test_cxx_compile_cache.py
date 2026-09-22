def test_cxx_compile_reuses_identical_formula(neuron_instance):
    """Identical reaction C++ is compiled once per process."""
    from neuron.rxd.rxd import _cxx_compile, _cxx_compile_stats

    formula = (
        "#include <math.h>\n"
        'extern "C" {\n'
        "double reaction(double* species, double* rhs) { return 0.0; }\n"
        "}\n"
    )
    n_compile = _cxx_compile_stats["compile"]
    n_hit = _cxx_compile_stats["hit"]
    first = _cxx_compile(formula)
    assert _cxx_compile_stats["compile"] == n_compile + 1
    assert _cxx_compile_stats["hit"] == n_hit
    second = _cxx_compile(formula)
    assert _cxx_compile_stats["compile"] == n_compile + 1
    assert _cxx_compile_stats["hit"] == n_hit + 1
    assert first is second
