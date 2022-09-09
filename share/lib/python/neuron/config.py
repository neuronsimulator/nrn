def _convert_value(key, value):
    """Convert a string representing a CMake variable value into a Python value.

    This does some basic conversion of values that CMake interprets as boolean
    values into Python's True/False, and includes some special cases for
    variables that are known to represent lists. See also:
    https://cmake.org/cmake/help/latest/command/if.html#basic-expressions.
    """
    if key.upper() in {"NRN_ENABLE_MODEL_TESTS"}:
        return tuple(value.split(";"))
    elif value.upper() in {"ON", "YES", "TRUE", "Y"}:
        return True
    elif value.upper() in {"OFF", "NO", "FALSE", "N"}:
        return False
    try:
        return int(value)
    except ValueError:
        return value


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
        arguments[key] = _convert_value(key, val)
