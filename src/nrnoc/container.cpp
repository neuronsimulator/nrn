#include "membfunc.h"
#include "neuron/container/generic_data_handle.hpp"
#include "neuron/container/soa_container.hpp"
#include "neuron/model_data.hpp"
#include "section.h"

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
    // needs some re-organisation if we ever want to support multiple Model instances
    assert(!container::detail::defer_delete_storage);
    container::detail::defer_delete_storage = &m_ptrs_for_deferred_deletion;
}
Model::~Model() {
    assert(container::detail::defer_delete_storage == &m_ptrs_for_deferred_deletion);
    container::detail::defer_delete_storage = nullptr;
    std::for_each(m_ptrs_for_deferred_deletion.begin(),
                  m_ptrs_for_deferred_deletion.end(),
                  [](void* ptr) { operator delete[](ptr); });
}
std::unique_ptr<container::utils::storage_info> Model::find_container_info(void const* cont) const {
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
    return {};
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
        auto* const container_data = *static_cast<void**>(dh.m_container);
        auto const maybe_info = utils::find_container_info(container_data);
        if (maybe_info) {
            if (!maybe_info->container().empty()) {
                os << "cont=" << maybe_info->container() << ' ';
            }
            os << maybe_info->field() << ' ' << dh.m_offset << '/' << maybe_info->size();
        } else {
            // couldn't find which container it points into; if container_data is null that will be
            // because the relevant container/column was deleted
            os << "cont=" << (container_data ? "unknown " : "deleted ") << dh.m_offset
               << "/unknown";
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
    return os << " type=" << dh.type_name() << '}';
}
}  // namespace neuron::container
namespace neuron::container::detail {
// See neuron/container/soa_container.hpp
std::vector<void*>* defer_delete_storage{};
}  // namespace neuron::container::detail
namespace neuron::container::Mechanism {
storage::storage(short mech_type, std::string name, std::vector<Variable> floating_point_fields)
    : base_type{field::FloatingPoint{std::move(floating_point_fields)}}
    , m_mech_name{std::move(name)}
    , m_mech_type{mech_type} {}
double& storage::fpfield(std::size_t instance, int field, int array_index) {
    return get_field_instance<field::FloatingPoint>(instance, field, array_index);
}
double const& storage::fpfield(std::size_t instance, int field, int array_index) const {
    return get_field_instance<field::FloatingPoint>(instance, field, array_index);
}
data_handle<double> storage::fpfield_handle(non_owning_identifier_without_container id,
                                            int field,
                                            int array_index) {
    return get_field_instance_handle<field::FloatingPoint>(id, field, array_index);
}
std::string_view storage::name() const {
    return m_mech_name;
}
short storage::type() const {
    return m_mech_type;
}
std::ostream& operator<<(std::ostream& os, storage const& data) {
    return os << data.name() << "::storage{type=" << data.type() << ", "
              << data.get_tag<field::FloatingPoint>().num_variables() << " fields}";
}
}  // namespace neuron::container::Mechanism
namespace neuron::container::utils {
namespace detail {
generic_data_handle promote_or_clear(generic_data_handle gdh) {
    // The whole point of this method is that it receives a raw pointer
    assert(!gdh.refers_to_a_modern_data_structure());
    auto& model = neuron::model();
    if (auto h = model.node_data().find_data_handle(gdh); h.refers_to_a_modern_data_structure()) {
        return h;
    }
    bool done{false};
    model.apply_to_mechanisms([&done, &gdh](auto& mech_data) {
        if (done) {
            return;
        }
        if (auto h = mech_data.find_data_handle(gdh); h.refers_to_a_modern_data_structure()) {
            gdh = std::move(h);
            done = true;
        }
    });
    if (done) {
        return gdh;
    }
    return {};
}
}  // namespace detail
std::unique_ptr<storage_info> find_container_info(void const* c) {
    return model().find_container_info(c);
}
}  // namespace neuron::container::utils
