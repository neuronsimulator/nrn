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
	IvocVect();
	IvocVect(int);
	IvocVect(int, double);
	IvocVect(IvocVect&);
	~IvocVect();

	void resize(int);
	void resize_chunk(int len, int realloc_extra = 0);
	int capacity();
	double& elem(int);
	double* vec();
	void label(const char*);

	int buffer_size();
	void buffer_size(int);

#if (USE_PTHREAD || defined(_OPENMP))
	void mutconstruct(int mkmut) {if (!mut_) MUTCONSTRUCT(mkmut)}
#else
	void mutconstruct(int) {}
#endif
	void lock() {MUTLOCK}
	void unlock() {MUTUNLOCK}
public:
	//intended as friend static Object** temp_objvar(IvocVect*);
	char* label_;
	int len;
	int space;
	double* s;
	MUTDEC
};


extern "C" {
extern IvocVect* vector_new(int); // use this if possible
extern IvocVect* vector_new0();
extern IvocVect* vector_new1(int);
extern void vector_delete(IvocVect*);
extern int vector_buffer_size(IvocVect*);
extern int vector_capacity(IvocVect*);
extern void vector_resize(IvocVect*, int);
extern double* vector_vec(IvocVect*);
extern char* vector_get_label(IvocVect*);
extern void vector_set_label(IvocVect*, char*);
}

inline IvocVect::IvocVect(){
  len = 0; s = 0; space = 0; label_ = 0;
}
                        
inline IvocVect::IvocVect(int l){
  s = new double [space = len = l]; label_ = 0;
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

inline int IvocVect::buffer_size(){
  return space;
}
                       	
inline IvocVect::~IvocVect(){
  delete [] s;
  if (label_) { delete [] label_; }
}

#endif

