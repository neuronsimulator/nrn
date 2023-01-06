#include "neuron/container/generic_data_handle.hpp"
#include "neuron/container/mechanism_data.hpp"
#include "nrnoc_ml.h"

#include <cassert>
#include <iterator>  // std::distance, std::next

Memb_list::Memb_list(int type)
    : m_storage{&neuron::model().mechanism_data(type)} {
    assert(type == m_storage->type());
}

[[nodiscard]] std::vector<double*> Memb_list::data() {
    assert(m_storage);
    assert(m_storage_offset != std::numeric_limits<std::size_t>::max());
    auto const num_fields = m_storage->num_floating_point_fields();
    std::vector<double*> ret(num_fields, nullptr);
    for (auto i = 0ul; i < num_fields; ++i) {
        ret[i] = &m_storage->get_field_instance<neuron::container::Mechanism::field::FloatingPoint>(
            i, m_storage_offset);
    }
    return ret;
}

[[nodiscard]] double& Memb_list::data(std::size_t instance, std::size_t variable) {
    assert(m_storage);
    assert(m_storage_offset != std::numeric_limits<std::size_t>::max());
    return m_storage->get_field_instance<neuron::container::Mechanism::field::FloatingPoint>(
        variable, m_storage_offset + instance);
}

[[nodiscard]] double const& Memb_list::data(std::size_t instance, std::size_t variable) const {
    assert(m_storage);
    assert(m_storage_offset != std::numeric_limits<std::size_t>::max());
    return m_storage->get_field_instance<neuron::container::Mechanism::field::FloatingPoint>(
        variable, m_storage_offset + instance);
}


[[nodiscard]] std::ptrdiff_t Memb_list::legacy_index(double const* ptr) const {
    assert(m_storage_offset != std::numeric_limits<std::size_t>::max());
    auto const size = m_storage->size();
    auto const num_fields = m_storage->num_floating_point_fields();
    for (auto field = 0ul; field < num_fields; ++field) {
        auto const* const vec_data =
            &m_storage->get_field_instance<neuron::container::Mechanism::field::FloatingPoint>(
                field, 0);
        auto const index = std::distance(vec_data, ptr);
        if (index >= 0 && index < size) {
            // ptr lives in the field-th data column
            return (index - m_storage_offset) * num_fields + field;
        }
    }
    // ptr doesn't live in this mechanism data, cannot compute a legacy index
    return -1;
}

[[nodiscard]] std::size_t Memb_list::num_floating_point_fields() const {
    assert(m_storage);
    return m_storage->num_floating_point_fields();
}

[[nodiscard]] double* Memb_list::pdata(std::size_t instance, std::size_t variable) {
    return _pdata[instance][variable].get<double*>();
}

[[nodiscard]] int Memb_list::type() const {
    assert(m_storage);
    return m_storage->type();
}
