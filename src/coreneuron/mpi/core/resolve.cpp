#include <dlfcn.h>
#include <sstream>
#include "../nrnmpi.h"

namespace coreneuron {
// Those functions are part of a mechanism to dynamically load mpi or not
void mpi_manager_t::resolve_symbols(void* lib_handle) {
    for (size_t i = 0; i < m_num_function_ptrs; ++i) {
        auto* ptr = m_function_ptrs[i];
        assert(!(*ptr));
        ptr->resolve(lib_handle);
        assert(*ptr);
    }
}

void mpi_function_base::resolve(void* lib_handle) {
    dlerror();
    void* ptr = dlsym(lib_handle, m_name);
    const char* error = dlerror();
    if (error) {
        std::ostringstream oss;
        oss << "Could not get symbol '" << m_name << "' from handle '" << lib_handle
            << "': " << error;
        throw std::runtime_error(oss.str());
    }
    assert(ptr);
    m_fptr = ptr;
}
}  // namespace coreneuron
