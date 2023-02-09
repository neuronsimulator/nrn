: This mechanism is intended to check for issues in .mod -> .cpp translation
NEURON	{
    SUFFIX NameClashes
    RANGE cache, container, data_handle, detail, ends_with_underscore_, field_index, generic_data_handle, legacy, mechanism, model_sorted_token, neuron, nrn, std
    THREADSAFE
}

ASSIGNED {
    cache
    container
    data_handle
    detail
    ends_with_underscore_
    field_index
    generic_data_handle
    legacy
    mechanism
    model_sorted_token
    neuron
    nrn
    std
}

BREAKPOINT {
    cache = 1
    container = 1
    data_handle = 1
    detail = 1
    ends_with_underscore_ = 1
    field_index = 1
    generic_data_handle = 1
    legacy = 1
    mechanism = 1
    model_sorted_token = 1
    neuron = 1
    nrn = 1
    std = 1
}
