#include <dlfcn.h>
#include <sstream>
#include "../nrnmpi.h"

namespace coreneuron {
// Those functions are part of a mechanism to dynamically load mpi or not
void mpi_manager_t::resolve_symbols(void* handle) {
    for (auto* ptr: m_function_ptrs) {
        assert(!(*ptr));
        ptr->resolve(handle);
        assert(*ptr);
    }
}

void mpi_function_base::resolve(void* handle) {
    dlerror();
    void* ptr = dlsym(handle, m_name);
    const char* error = dlerror();
    if (error) {
        std::ostringstream oss;
        oss << "Could not get symbol " << m_name << " from handle " << handle << ": " << error;
        throw std::runtime_error(oss.str());
    }
    assert(ptr);
    m_fptr = ptr;
}
}  // namespace coreneuron
