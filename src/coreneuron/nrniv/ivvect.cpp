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

IvocVect* vector_new(int n){return new IvocVect(n);}
IvocVect* vector_new1(int n){return new IvocVect(n);}
int vector_capacity(IvocVect* v){return v->capacity();}
double* vector_vec(IvocVect* v){return v->vec();}

