#include <nrnconf.h>
#include <ivocvect.h>
#include <ivlist.h>
#include <string.h>

void IvocVect::resize(int newl) { // all that for this
        long oldcap = capacity();

 if (newl > space) {
  double* news = new double [newl];
  double* p = news;
  int minl = (len < newl)? len : newl;
  double* top = &(s[minl]);
  double* t = s;
  while (t < top) *p++ = *t++;
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

void IvocVect::label(const char* label) {
        if (label_) {
                delete [] label_;
                label_ = nil;
        }
        if (label) {
                label_ = new char[strlen(label) + 1];   
                strcpy(label_, label);
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

