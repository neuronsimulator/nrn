#pragma once
#if USE_PTHREAD

// #ifdef MINGW
// #undef DELETE
// #undef near
// #endif

#include <memory>
#include <mutex>

#define MUTDEC         std::unique_ptr<std::mutex> mut_;
#define MUTCONSTRUCTED bool{mut_}
#define MUTCONSTRUCT(mkmut)              \
        { if (mkmut) {                     \
            mut_ = std::make_unique<std::mutex>();     \
        } else {                         \
            mut_.reset();              \
        } }
#define MUTDESTRUCT mut_.reset();
#define MUTLOCK                       \
        { if (mut_) {                   \
            mut_->lock(); \
        } }
#define MUTUNLOCK                       \
        { if (mut_) {                     \
            mut_->unlock(); \
        } }
#else
#define MUTDEC              /**/
#define MUTCONSTRUCTED      (0)
#define MUTCONSTRUCT(mkmut) /**/
#define MUTDESTRUCT         /**/
#define MUTLOCK             /**/
#define MUTUNLOCK           /**/
#endif
