/*
Copyright (c) 2014 EPFL-BBP, All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE BLUE BRAIN PROJECT "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE BLUE BRAIN PROJECT
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <string.h>
#include "coreneuron/nrnconf.h"
#include "coreneuron/nrniv/ivocvect.h"
#include "coreneuron/nrniv/ivlist.h"

void IvocVect::resize(int newl) { // all that for this
        long oldcap = capacity();

 if (newl > space) {
  double* news = new double [newl];
  double* p = news;
  int minl = (len < newl)? len : newl;
  double* top = &(s[minl]);
  double* tt = s;
  while (tt < top) *p++ = *tt++;
  delete [] s;
  s = news;
  space = newl;
 }
  len = newl;

        for (;oldcap < newl; ++oldcap) {
                elem(oldcap) = 0.;
        }
}

void IvocVect::resize_chunk(int newlen, int extra) {
        if (newlen > space) {
                long x = newlen + extra;
                if (extra == 0) {
                        x = ListImpl_best_new_count(newlen, sizeof(double));
                }
//              printf("resize_chunk %d\n", x);
                resize(x);
        }
        resize(newlen);
}

void IvocVect::buffer_size(int n) {
        double* y = new double[n];
        if (len > n) {
                len = n;
        }
        for (int i=0; i < len; ++i) {
                y[i] = s[i];
        }
        space = n;   
        delete [] s;
        s = y;
}

void IvocVect::label(const char* lab) {
        if (label_) {
                delete [] label_;
                label_ = nil;
        }
        if (lab) {
                label_ = new char[strlen(lab) + 1];   
                strcpy(label_, lab);
        }   
}

IvocVect* vector_new(int n){return new IvocVect(n);}
IvocVect* vector_new0(){return new IvocVect();}
IvocVect* vector_new1(int n){return new IvocVect(n);}
void vector_delete(IvocVect* v){delete v;}
int vector_buffer_size(IvocVect* v){return v->buffer_size();}
int vector_capacity(IvocVect* v){return v->capacity();}
void vector_resize(IvocVect* v, int n){v->resize(n);}
double* vector_vec(IvocVect* v){return v->vec();}
char* vector_get_label(IvocVect* v) { return v->label_; }
void vector_set_label(IvocVect* v, char* s) { v->label(s); }

