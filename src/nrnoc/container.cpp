#include "membfunc.h"
#include "neuron/container/callbacks.hpp"
#include "neuron/container/generic_data_handle.hpp"
#include "neuron/model_data.hpp"

#include <cstddef>
#include <optional>

namespace {
void invalidate_cache() {
    neuron::cache::model.reset();
}
}  // namespace
namespace neuron {
Model::Model() {
    m_node_data.set_unsorted_callback(invalidate_cache);
}
std::optional<container::utils::storage_info> Model::find_container_info(void const* cont) const {
    if (auto maybe_info = m_node_data.find_container_info(cont); maybe_info) {
        return maybe_info;
    }
    for (auto& mech_data: m_mech_data) {
        if (!mech_data) {
            continue;
        }
        if (auto maybe_info = mech_data->find_container_info(cont); maybe_info) {
            return maybe_info;
        }
    }
    return {std::nullopt};
}

void Model::set_unsorted_callback(container::Mechanism::storage& mech_data) {
    mech_data.set_unsorted_callback(invalidate_cache);
    // This is called when a new Mechanism storage struct is created, i.e. when
    // a new Mechanism type is registered. When that happens the cache
    // implicitly becomes invalid, because it does not contain entries for the
    // newly-added Mechanism. If this proves to be a bottleneck then we could
    // handle this more efficiently.
    invalidate_cache();
}
}  // namespace neuron
namespace neuron::detail {
// See neuron/model_data.hpp
Model model_data;
}  // namespace neuron::detail
namespace neuron::cache {
std::optional<Model> model{};
}
namespace neuron::container {
void ptr_update_data::prepare() {
    // Make sure the containers are sorted by the first member of the tuple
    std::sort(m_permutations.begin(), m_permutations.end(), [](auto const& lhs, auto const& rhs) {
        return std::get<0>(lhs) < std::get<0>(rhs);
    });
    std::sort(m_reallocations.begin(), m_reallocations.end(), [](auto const& lhs, auto const& rhs) {
        return std::get<0>(lhs) < std::get<0>(rhs);
    });
}
void* ptr_update_data::recalculate_ptr(void* ptr, std::size_t sizeof_ptr_type) const {
    auto* const byte_ptr = static_cast<std::byte*>(ptr);
    // the first element of each row in container is the starting address of the range
    // represented by that row
    auto lookup = [byte_ptr](auto const& container) {
        if (container.empty()) {
            // the container is empty => no match
            return container.end();
        }
        // first iterator greater than byte_ptr, or last if not found
        auto const iter = std::upper_bound(container.begin(),
                                           container.end(),
                                           byte_ptr,
                                           [](std::byte* ptr, auto const& entry) {
                                               return ptr < std::get<0>(entry);
                                           });
        if (iter == container.begin()) {
            // the first range starts after byte_ptr => no match
            return container.end();
        } else {
            // this should be the iterator to the range byte_ptr might fall in
            return std::prev(iter);
        }
    };
    if (auto const iter = lookup(m_permutations); iter != m_permutations.end()) {
        auto const& [data, perm, size_of_type, size_of_data] = *iter;
        assert(byte_ptr >= data);
        if (byte_ptr < data + size_of_type * size_of_data) {
            assert(sizeof_ptr_type == size_of_type);
            assert((byte_ptr - data) % size_of_type == 0);
            auto const old_index = (byte_ptr - data) / size_of_type;
            assert(old_index < size_of_data);
            auto const new_index = perm[old_index];
            return data + new_index * size_of_type;
        }
    }
    if (auto const iter = lookup(m_reallocations); iter != m_reallocations.end()) {
        auto const& [old_data, new_data, size_of_type, old_size] = *iter;
        assert(byte_ptr >= old_data);
        if (byte_ptr < old_data + size_of_type * old_size) {
            assert(sizeof_ptr_type == size_of_type);
            assert((byte_ptr - old_data) % size_of_type == 0);
            auto const index = (byte_ptr - old_data) / size_of_type;
            return new_data + index * size_of_type;
        }
    }
    return ptr;
}

std::ostream& operator<<(std::ostream& os, generic_data_handle const& dh) {
    os << "generic_data_handle{";
    if (!dh.m_offset.has_always_been_null()) {
        // modern and valid or once-valid data handle
        auto const maybe_info = utils::find_container_info(dh.m_container);
        if (maybe_info) {
            if (!maybe_info->container.empty()) {
                os << "cont=" << maybe_info->container << ' ';
            }
            os << maybe_info->field << ' ' << dh.m_offset << '/' << maybe_info->size;
        } else {
            // couldn't find which container it points into
            os << "cont=unknown " << dh.m_offset << "/unknown";
        }
    } else {
        // legacy data handle
        os << "raw=";
        if (dh.m_container) {
            // This shouldn't crash, but it might contain some garbage if
            // we're wrapping a literal value
            os << dh.m_container;
        } else {
            os << "nullptr";
        }
    }
    return os << ", type=" << dh.type_name() << '}';
}
}  // namespace neuron::container
namespace neuron::container::detail {
// See neuron/container/callbacks.hpp
ptr_update_data ptr_transformations;
std::vector<ptr_update_callback_t> ptr_update_callbacks;
// See neuron/container/soa_identifier.hpp
std::vector<std::unique_ptr<std::size_t>> garbage;
}  // namespace neuron::container::detail

namespace neuron::container::Mechanism {
storage::storage(short mech_type, std::string name, std::size_t num_floating_point_fields)
    : base_type{field::FloatingPoint{num_floating_point_fields}}
    , m_mech_name{std::move(name)}
    , m_mech_type{mech_type} {}
}  // namespace neuron::container::Mechanism