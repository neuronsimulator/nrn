/**
 * @file sdprintf.h
 * @date 19th Jan 2015
 * @brief Header providing interface to dynamically allocating
 *  sprintf wrapper.
 *
 * Provides sprintf()-like function that uses the  offered character
 * array if it is sufficiently large, else allocates space on the heap.
 * 
 * The return object is a 'smart' pointer wrapper, offering a subset of
 * the C++11 std::unique_ptr functionality. (We need C++03
 * compatibility, and do not want to introduce a dependency on the
 * Boost library.)
 * 
 * Code assumes C99-compatible behaviour of (non-standard before C++11)
 * vsnprintf().  Microsoft Visual C++ does not conform, for example.
 */


#ifndef SDPRINTF_H_
#define SDPRINTF_H_

#include <cstdio>
#include <cstdlib>
#include <cstdarg>

/** @brief 'Smart' pointer wrapper for char * buffers
 *
 * Assignment and copies preserve the value
 * of the contained pointer, but the last assignee
 * holds responsibility for deallocation of the
 * buffer.
 */

template <void (*dealloc)(void *)>
struct sd_ptr_generic {
    sd_ptr_generic(): ptr(0),dflag(false) {}

    sd_ptr_generic(const char *p,bool dflag_=false): ptr(p),dflag(dflag_) {}

    sd_ptr_generic(const sd_ptr_generic &them): ptr(them.ptr),dflag(them.dflag) {
        them.dflag=false;
    }

    sd_ptr_generic &operator=(const char *p) {
        release();
        ptr=p;
        return *this;
    }

    sd_ptr_generic &operator=(const sd_ptr_generic &them) {
        if (&them!=this) {
            release();
            ptr=them.ptr;
            dflag=them.dflag;
            them.dflag=false;
        }
        return *this;
    }

    void release() {
        if (dflag) dealloc((void *)ptr);
        dflag=false;
    }

    const char *get() const { return ptr; }
    operator const char *() const { return get(); }

    operator bool() const { return (bool)ptr; }

    ~sd_ptr_generic() { release(); }

private:
    const char *ptr;
    mutable bool dflag;
};

typedef sd_ptr_generic<std::free> sd_ptr;

/** @brief Dynamically allocating snprintf()
 *
 * If the provided buffer is too small, sdprintf()
 * allocates one of the appropriate size.
 *
 * @param buf Pointer to buffer in which to write string.
 * @param sz  Size in bytes of buffer.
 * @param fmt A printf format string.
 *
 * @return An sd_ptr encapsulating the provided or allocated buffer.
 *
 * If the buffer pointer is NULL, a new buffer is allocated.
 * On error, returns a null sd_ptr.
 */

sd_ptr sdprintf(char *buf,size_t sz,const char *fmt,...);

/** @brief Varargs version of sdprintf() (q.v.)
 * 
 * @param buf Pointer to buffer in which to write string.
 * @param sz  Size in bytes of buffer.
 * @param fmt A printf format string.
 * @param ap  A varargs list of printf arguments.
 *
 * @return An sd_ptr encapsulating the provided or allocated buffer.
 */

sd_ptr vsdprintf(char *buf,size_t sz,const char *fmt,va_list ap);

#endif // ndef SDPRINTF_H_
