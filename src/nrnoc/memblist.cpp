#include "neuron/container/generic_data_handle.hpp"
#include "neuron/container/mechanism_data.hpp"
#include "neuron/model_data.hpp"
#include "nrnoc_ml.h"

#include <cassert>
#include <iterator>  // std::distance, std::next
#include <numeric>   // std::accumulate

Memb_list::Memb_list(int type)
    : m_storage{&neuron::model().mechanism_data(type)} {
    assert(type == m_storage->type());
}

[[nodiscard]] std::vector<double*> Memb_list::data() {
    using Tag = neuron::container::Mechanism::field::FloatingPoint;
    assert(m_storage);
    assert(m_storage_offset != neuron::container::invalid_row);
    auto const num_fields = m_storage->get_tag<Tag>().num_variables();
    std::vector<double*> ret(num_fields, nullptr);
    for (auto i = 0; i < num_fields; ++i) {
        ret[i] = &m_storage->get_field_instance<Tag>(m_storage_offset, i);
    }
    return ret;
}

neuron::container::data_handle<double> Memb_list::data_handle(
    std::size_t instance,
    neuron::container::field_index field) const {
    assert(m_storage);
    assert(m_storage_offset != neuron::container::invalid_row);
    auto const offset = m_storage_offset + instance;
    using Tag = neuron::container::Mechanism::field::FloatingPoint;
    return m_storage->get_field_instance_handle<Tag>(m_storage->get_identifier(offset),
                                                     field.field,
                                                     field.array_index);
}

[[nodiscard]] double& Memb_list::data(std::size_t instance, int variable, int array_index) {
    assert(m_storage);
    assert(m_storage_offset != neuron::container::invalid_row);
    return m_storage->get_field_instance<neuron::container::Mechanism::field::FloatingPoint>(
        m_storage_offset + instance, variable, array_index);
}

[[nodiscard]] double const& Memb_list::data(std::size_t instance,
                                            int variable,
                                            int array_index) const {
    assert(m_storage);
    assert(m_storage_offset != neuron::container::invalid_row);
    return m_storage->get_field_instance<neuron::container::Mechanism::field::FloatingPoint>(
        m_storage_offset + instance, variable, array_index);
}


[[nodiscard]] std::ptrdiff_t Memb_list::legacy_index(double const* ptr) const {
    assert(m_storage_offset != neuron::container::invalid_row);
    // For a mechanism with (in order) range variables: a, b[2], c the mechanism data are
    //              ______________________
    //  instance 0 | a   b[0]   b[1]   c   |
    //  instance 1 | a'  b'[0]  b'[1]  c'  |
    //  instance 2 | a'' b''[0] b''[1] c'' |
    //
    // the old layout arranged this as:
    //  [a b[0], b[1], c, a', b'[0], b'[1], c', a'', b''[0], b''[1], c'']
    // whereas the new layout has three different storage vectors:
    //  [a, a', a'']
    //  [b[0], b[1], b'[0], b'[1], b''[0], b''[1]]
    //  [c, c', c'']
    // this method, given a pointer into one of the new layout vectors,
    // calculates the (hypothetical) index into the old (single) vector
    using Tag = neuron::container::Mechanism::field::FloatingPoint;
    auto const size = m_storage->size();  // number of instances; 3 in the example above
    auto const num_fields = m_storage->get_tag<Tag>().num_variables();  // ex: 3 (a, b, c)
    auto const* const array_dims = m_storage->get_array_dims<Tag>();    // ex: [1, 2, 1]
    auto const sum_of_array_dims = std::accumulate(array_dims, array_dims + num_fields, 0);
    int sum_of_array_dims_of_previous_fields{};
    for (auto field = 0; field < num_fields; ++field) {  // a, b or c in the example above
        auto const array_dim = array_dims[field];
        assert(array_dim > 0);
        auto const* const vec_data = &m_storage->get_field_instance<Tag>(0, field);
        auto const index = std::distance(vec_data, ptr);
        // storage vectors are size * array_dim long
        if (index >= 0 && index < size * array_dim) {
            auto const instance_offset = index / array_dim;
            auto const array_index = index % array_dim;
            assert(ptr == &m_storage->get_field_instance<Tag>(instance_offset, field, array_index));
            return ((instance_offset - m_storage_offset) * sum_of_array_dims) +
                   sum_of_array_dims_of_previous_fields + array_index;
        }
        sum_of_array_dims_of_previous_fields += array_dim;
    }
    assert(sum_of_array_dims_of_previous_fields == sum_of_array_dims);
    // ptr doesn't live in this mechanism data, cannot compute a legacy index
    return -1;
}

[[nodiscard]] double* Memb_list::dptr_field(std::size_t instance, int variable) {
    return pdata[instance][variable].get<double*>();
}

[[nodiscard]] int Memb_list::type() const {
    assert(m_storage);
    return m_storage->type();
}
