def _parse_arguments(h):
    """Map the C++ structure neuron::config::arguments into Python.

    The Python version is accessible as neuron.config.arguments.
    """
    global arguments
    arguments = {}
    num_keys_double = h.nrn_num_config_keys()
    num_keys = int(num_keys_double)
    assert float(num_keys) == num_keys_double
    for i in range(num_keys):
        key = h.nrn_get_config_key(i)
        val = h.nrn_get_config_val(i)
        assert key not in arguments
        if key == "NRN_ENABLE_MODEL_TESTS":
            val = tuple(val.split(";"))
        val = {"ON": True, "OFF": False}.get(val, val)
        arguments[key] = val
