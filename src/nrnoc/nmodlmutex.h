#pragma once
#if NRN_ENABLE_THREADS
#include <memory>
#include <mutex>

namespace nrn {
extern std::unique_ptr<std::mutex> nmodlmutex;
}
#define _NMODLMUTEXLOCK              \
    {                                \
        if (nrn::nmodlmutex) {       \
            nrn::nmodlmutex->lock(); \
        }                            \
    }
#define _NMODLMUTEXUNLOCK              \
    {                                  \
        if (nrn::nmodlmutex) {         \
            nrn::nmodlmutex->unlock(); \
        }                              \
    }
#else
#define _NMODLMUTEXLOCK   /**/
#define _NMODLMUTEXUNLOCK /**/
#endif
