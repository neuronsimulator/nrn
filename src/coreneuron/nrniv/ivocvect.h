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

#ifndef ivoc_vector_h
#define ivoc_vector_h

#include "coreneuron/nrniv/nrnmutdec.h"

class IvocVect {
public:
    int len;
    int space;
    MUTDEC
    double* s;

    IvocVect();
    IvocVect(int);
	~IvocVect();

	void resize(int);
	int capacity();
	double& elem(int);
	double* vec();
#if (USE_PTHREAD || defined(_OPENMP))
	void mutconstruct(int mkmut) {if (!mut_) MUTCONSTRUCT(mkmut)}
#else
	void mutconstruct(int) {}
#endif
	void lock() {MUTLOCK}
	void unlock() {MUTUNLOCK}
};


extern "C" {
extern IvocVect* vector_new(int); // use this if possible
extern IvocVect* vector_new1(int);
extern int vector_capacity(IvocVect*);
extern double* vector_vec(IvocVect*);
}

inline IvocVect::IvocVect(){
  len = 0; s = 0; space = 0;
}
                        
inline IvocVect::IvocVect(int l){
  s = new double [space = len = l];
}

inline double* IvocVect::vec() {
  return s;
}
inline double& IvocVect::elem(int n) {
  return s[n];
}

inline int IvocVect::capacity(){
  return len;
}

                       	
inline IvocVect::~IvocVect(){
  delete [] s;
}

#endif

