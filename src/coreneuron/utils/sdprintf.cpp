/*
Copyright (c) 2016, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

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

#if !defined(va_copy)
/* check for __va_copy: work around for icpc 2015 */
#if defined(__va_copy)
#define va_copy(dest,src) __va_copy(dest,src)
#else 
/* non-portable, so specialise for those cases where
 *  * value assignment does not work and va_copy is nonetheless
 *   * not defined */
#warning "no va_copy() or __va_copy defined, using value assignment"
#define va_copy(dest,src) ((dest)=(src))
#endif
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
        char *p=(char *)malloc(sz);
        if (!p){
            va_end(ap2);
            return 0;
        }

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
