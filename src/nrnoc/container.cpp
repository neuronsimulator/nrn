#include "membfunc.h"
#include "neuron/model_data.hpp"

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
// See neuron/container/soa_identifier.hpp
std::vector<std::unique_ptr<std::size_t>> garbage;
}  // namespace neuron::container::detail

namespace neuron::container::Mechanism {
storage::storage(short mech_type, std::string name, std::size_t num_floating_point_fields)
    : base_type{field::FloatingPoint{num_floating_point_fields}}
    , m_mech_name{std::move(name)}
    , m_mech_type{mech_type} {}
}  // namespace neuron::container::Mechanism