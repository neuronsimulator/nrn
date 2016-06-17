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


#define BOOST_TEST_MODULE sdprintf
#define BOOST_TEST_MAIN

#include <cstring>
#include <cwchar>
#include <clocale>
#include <map>

#include <iostream>

#include <boost/test/unit_test.hpp>

#include "coreneuron/utils/sdprintf.h"

BOOST_AUTO_TEST_CASE(sdprintf_noalloc) {
    char buf[10]="";
    sd_ptr s=sdprintf(buf,sizeof(buf),"abc%s","def");

    BOOST_CHECK(s==buf);
    BOOST_CHECK(!strcmp(s,"abcdef"));

    // check edge case: string to write (with NUL) is size of buffer
    s=sdprintf(buf,sizeof(buf),"abcdefghi");
    BOOST_CHECK(s==buf);
    BOOST_CHECK(!strcmp(s,"abcdefghi"));
}

bool make_invalid_wchar(wchar_t &w) {
    std::setlocale(LC_ALL,"C");
    w=(wchar_t)WEOF;

    std::mbstate_t ps=std::mbstate_t();
    char s[MB_LEN_MAX];
    size_t r=wcrtomb(s,w,&ps);
    return r==(size_t)-1;
}

BOOST_AUTO_TEST_CASE(sdprintf_error) {
    char buf[10]="";
    wchar_t invalid;
    if (make_invalid_wchar(invalid)) {
        sd_ptr s=sdprintf(buf,sizeof(buf),"%lc",(wint_t)invalid);
        BOOST_CHECK(!s);
    }
    else {
        BOOST_WARN("unable to check error return with invalid conversion");
    }
}

BOOST_AUTO_TEST_CASE(sdprintf_nullbuf) {
    sd_ptr s=sdprintf(0,0,"%s","hello");
    BOOST_CHECK(!strcmp(s,"hello"));
}

BOOST_AUTO_TEST_CASE(sdprintf_alloc) {
    char buf[10]="";
    sd_ptr s=sdprintf(0,0,"abc%s","defghijkl");
    BOOST_CHECK(!strcmp(s,"abcdefghijkl"));
    BOOST_CHECK(s!=buf);
}

BOOST_AUTO_TEST_CASE(sd_ptr_ctor) {
    char p[1];

    sd_ptr a;
    BOOST_CHECK(!a);
    BOOST_CHECK(a==0);

    sd_ptr b(0);
    BOOST_CHECK(!b);
    BOOST_CHECK(b==0);

    sd_ptr c(p);
    BOOST_CHECK(c);
    BOOST_CHECK(c==p);

    sd_ptr d(p,false);
    BOOST_CHECK(d);
    BOOST_CHECK(d==p);

    sd_ptr e(d);
    BOOST_CHECK(d==p);
    BOOST_CHECK(e==p);
}

BOOST_AUTO_TEST_CASE(sd_ptr_assign) {
    char p[1],q[1];
    sd_ptr a,b,c;

    a=p;
    b=q;
    c=0;

    BOOST_CHECK(a==p);
    BOOST_CHECK(b==q);
    BOOST_CHECK(c==0);

    a=b;
    c=b;

    BOOST_CHECK(a==q);
    BOOST_CHECK(c==q);
}

    
BOOST_AUTO_TEST_CASE(sd_ptr_get) {
    char p[1];

    sd_ptr a(p);
    BOOST_CHECK(a.get()==p);
    BOOST_CHECK(a==p);
    BOOST_CHECK(a);

    sd_ptr z(0);
    BOOST_CHECK(z.get()==0);
    BOOST_CHECK(z==0);
    BOOST_CHECK(!z);
}

std::map<void *,int> nfree;
int nfree_total=0;

void reset_free_count() {
    nfree.clear();
    nfree_total=0;
}

void test_free(void *p) {
    ++nfree[p];
    ++nfree_total;
}

BOOST_AUTO_TEST_CASE(sd_ptr_dealloc) {
    typedef sd_ptr_generic<test_free> mock_sd_ptr;
    char p[1],q[1],r[1];

    reset_free_count();
    {
        mock_sd_ptr a(p),b(p,false);
        BOOST_CHECK(a==p);
        BOOST_CHECK(b==p);
    }
    BOOST_CHECK(nfree[p]==0);
    BOOST_CHECK(nfree_total==0);

    reset_free_count();
    mock_sd_ptr a(p,true);
    BOOST_CHECK(a==p);
    a.release();
    BOOST_CHECK(nfree[p]==1);
    BOOST_CHECK(nfree_total==1);

    reset_free_count();
    {
        mock_sd_ptr a(p,true);
        BOOST_CHECK(a==p);
    }
    BOOST_CHECK(nfree[p]==1);
    BOOST_CHECK(nfree_total==1);

    reset_free_count();
    {
        mock_sd_ptr a(p,true);
        {
            mock_sd_ptr b(a);
            BOOST_CHECK(nfree[p]==0);
        }
        BOOST_CHECK(nfree[p]==1);
    }
    BOOST_CHECK(nfree[p]==1);
    BOOST_CHECK(nfree_total==1);

    reset_free_count();
    {
        mock_sd_ptr a(p,true);
        mock_sd_ptr b(q,true);
        mock_sd_ptr c(r,false);
        BOOST_CHECK(a==p);
        BOOST_CHECK(b==q);
        BOOST_CHECK(c==r);
        b=a;
        BOOST_CHECK(b==p);
        BOOST_CHECK(nfree[p]==0);
        BOOST_CHECK(nfree[q]==1);
        c=a;
        BOOST_CHECK(c==p);
        BOOST_CHECK(nfree[p]==0);
        BOOST_CHECK(nfree[r]==0);
        a=a;
        BOOST_CHECK(a==p);
        BOOST_CHECK(nfree[p]==0);
    }
    BOOST_CHECK(nfree[p]==1);
    BOOST_CHECK(nfree_total==2);
}

    
