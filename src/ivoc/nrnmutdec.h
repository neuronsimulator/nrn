#pragma once
#if NRN_ENABLE_THREADS

#include <memory>
#include <mutex>

#define MUTDEC         std::unique_ptr<std::recursive_mutex> mut_;
#define MUTCONSTRUCTED static_cast<bool>(mut_)
#define MUTCONSTRUCT(mkmut)                                  \
    {                                                        \
        if (mkmut) {                                         \
            mut_ = std::make_unique<std::recursive_mutex>(); \
        } else {                                             \
            mut_.reset();                                    \
        }                                                    \
    }
#define MUTDESTRUCT mut_.reset();
#define MUTLOCK           \
    {                     \
        if (mut_) {       \
            mut_->lock(); \
        }                 \
    }
#define MUTUNLOCK           \
    {                       \
        if (mut_) {         \
            mut_->unlock(); \
        }                   \
    }
#else
#define MUTDEC              /**/
#define MUTCONSTRUCTED      false
#define MUTCONSTRUCT(mkmut) /**/
#define MUTDESTRUCT         /**/
#define MUTLOCK             /**/
#define MUTUNLOCK           /**/
#endif
