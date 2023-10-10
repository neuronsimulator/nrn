#include <neuron/model_data.hpp>
#include <neuron/cache/model_data.hpp>
#include <stdexcept>
#include "oc_ansi.h"

namespace neuron::container {
ModelMemoryUsage memory_usage(const Model& model) {
    auto nodes = memory_usage(model.node_data());

    auto mechanisms = StorageMemoryUsage();
    model.apply_to_mechanisms([&mechanisms](const auto& md) { mechanisms += memory_usage(md); });

    return {nodes, mechanisms};
}

cache::ModelMemoryUsage memory_usage(const std::optional<neuron::cache::Model>& model) {
    return model ? memory_usage(*model) : cache::ModelMemoryUsage{};
}
cache::ModelMemoryUsage memory_usage(const neuron::cache::Model& model) {
    VectorMemoryUsage thread(model.thread);
    for (const auto& t: model.thread) {
        thread += VectorMemoryUsage(t.mechanism_offset);
    }

    VectorMemoryUsage mechanism(model.mechanism);
    for (const auto& m: model.mechanism) {
        mechanism += VectorMemoryUsage(m.pdata_ptr_cache);

        mechanism += VectorMemoryUsage(m.pdata);
        for (const auto& pd: m.pdata) {
            mechanism += VectorMemoryUsage(pd);
        }

        mechanism += VectorMemoryUsage(m.pdata_hack);
        for (const auto& pdd: m.pdata_hack) {
            mechanism += VectorMemoryUsage(pdd);
        }
    }

    return {thread, mechanism};
}

MemoryUsage local_memory_usage() {
    return MemoryUsage{memory_usage(model()),
                       memory_usage(neuron::cache::model),
                       detail::compute_defer_delete_storage_size()};
}

namespace detail {
template <class T>
VectorMemoryUsage compute_defer_delete_storage_size(std::vector<T> const* const v,
                                                    size_t per_element_size) {
    if (v) {
        size_t size = v->size() * (sizeof(T) + per_element_size);
        size_t capacity = size + (v->capacity() - v->size()) * sizeof(T);

        return {size, capacity};
    }

    return {0ul, 0ul};
}


VectorMemoryUsage compute_defer_delete_storage_size() {
    return compute_defer_delete_storage_size(defer_delete_storage, sizeof(void*));
}
}  // namespace detail


/** @brief Format the memory sizes as human readable strings.
 *
 *  Currently, the intended use is in aligned tables in the memory usage
 *  report. Hence, the string has two digits and is 9 characters long
 *  (without the null char).
 */
std::string format_memory(size_t bytes) {
    static std::vector<std::string> suffixes{"   ", " kB", " MB", " GB", " TB", " PB"};

    size_t suffix_id = std::min(size_t(std::floor(std::log10(std::max(1.0, double(bytes))))) / 3,
                                suffixes.size() - 1);
    auto suffix = suffixes[suffix_id];

    double value = double(bytes) / std::pow(10.0, suffix_id * 3.0);

    char formatted[64];
    if (suffix_id == 0) {
        snprintf(formatted, sizeof(formatted), "% 6ld", bytes);
    } else {
        snprintf(formatted, sizeof(formatted), "%6.2f", value);
    }

    return formatted + suffix;
}

std::string format_memory_usage(const VectorMemoryUsage& memory_usage) {
    return format_memory(memory_usage.size) + "     " + format_memory(memory_usage.capacity);
}

std::string format_memory_usage(const MemoryUsage& usage) {
    const auto& model = usage.model;
    const auto& cache_model = usage.cache_model;
    const auto& stable_pointers = usage.stable_pointers;
    const auto& total = usage.compute_total();
    const auto& summary = MemoryUsageSummary(usage);

    std::stringstream os;

    os << "                             size     capacity \n";
    os << "Model \n";
    os << "  nodes \n";
    os << "    data                " << format_memory_usage(model.nodes.heavy_data) << "\n";
    os << "    stable_identifiers  " << format_memory_usage(model.nodes.stable_identifiers) << "\n";
    os << "  mechanisms \n";
    os << "    data                " << format_memory_usage(model.mechanisms.heavy_data) << "\n";
    os << "    stable_identifiers  " << format_memory_usage(model.mechanisms.stable_identifiers)
       << "\n";
    os << "cache::Model \n";
    os << "  threads               " << format_memory_usage(cache_model.threads) << "\n";
    os << "  mechanisms            " << format_memory_usage(cache_model.mechanisms) << "\n";
    os << "deferred deletion \n";
    os << "  stable_pointers       " << format_memory_usage(stable_pointers) << "\n";
    os << "\n";
    os << "total                   " << format_memory_usage(total) << "\n";
    os << "\n";
    os << "Summary\n";
    os << "  required              " << format_memory(summary.required) << "\n";
    os << "  convenient            " << format_memory(summary.convenient) << "\n";
    os << "  oversized             " << format_memory(summary.oversized) << "\n";
    os << "  leaked                " << format_memory(summary.leaked) << "\n";


    return os.str();
}


void print_memory_usage(MemoryUsage const& memory_usage) {
    std::cout << format_memory_usage(memory_usage) << "\n";
}

}  // namespace neuron::container

void print_local_memory_usage() {
    if (!ifarg(0)) {
        hoc_execerror("print_local_memory_usage doesn't support any arguments.", nullptr);
    }

    neuron::container::print_memory_usage(neuron::container::local_memory_usage());

    hoc_retpushx(1.);
}
