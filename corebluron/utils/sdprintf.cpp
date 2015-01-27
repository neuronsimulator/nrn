/**
 * @file sdprintf.cpp
 * @date 19th Jan 2015
 * @brief sdprintf() and vsdprintf() implementation.
 */

#include "sdprintf.h"

sd_ptr sdprintf(char *buf,size_t sz,const char *fmt,...) {
    va_list ap;
    va_start(ap,fmt);

    sd_ptr s=vsdprintf(buf,sz,fmt,ap);

    va_end(ap);
    return s;
}

#ifndef va_copy
/* non-portable, so specialise for those cases where
 * value assignment does not work and va_copy is nonetheless
 * not defined */
#warning "no va_copy() defined, using value assignment"
#define va_copy(dest,src) ((dest)=(src))
#endif

sd_ptr vsdprintf(char *buf,size_t sz,const char *fmt,va_list ap) {
    using namespace std;

    sd_ptr s;
    va_list ap2;
    va_copy(ap2,ap);

    int rv=0;
    if (buf!=0 && sz>0)
        rv=vsnprintf(buf,sz,fmt,ap);
    else {
        char p[1];
        sz=0;
        rv=vsnprintf(p,sz,fmt,ap);
    }

    if (rv<0) {
        s=0;
        goto exit;
    }

    if ((size_t)rv<sz) {
        s=buf;
        goto exit;
    }
    else {
        // buffer too small; allocate and try again
        sz=(size_t)rv+1;
        char *p=(char *)std::malloc(sz);
        if (!p) return 0;

        rv=vsnprintf(p,sz,fmt,ap2);
        if (rv<0 || (size_t)rv>=sz) {
            free(p);
            s=0;
            goto exit;
        }

        s=sd_ptr(p,true);
        goto exit;
    }

exit:
    va_end(ap2);
    return s;
}
