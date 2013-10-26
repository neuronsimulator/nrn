#ifndef ivoc_vector_h
#define ivoc_vector_h

#include <nrnmutdec.h>

class IvocVect {
public:
	IvocVect();
	IvocVect(int);
	IvocVect(int, double);
	IvocVect(IvocVect&);
	~IvocVect();

	void resize(int);
	void resize_chunk(int len, int realloc_extra = 0);
	// volatile, only one at a time, don't do anything
	// that changes the memory space.
	IvocVect* subvec(int start, int end);
	long capacity();
	double& elem(int);

	int buffer_size();
	void buffer_size(int);

#if USE_PTHREAD
	void mutconstruct(int mkmut) {if (!mut_) MUTCONSTRUCT(mkmut)}
#else
	void mutconstruct(int) {}
#endif
	void lock() {MUTLOCK}
	void unlock() {MUTUNLOCK}
public:
	//intended as friend static Object** temp_objvar(IvocVect*);
	char* label_;
	MUTDEC
};


extern "C" {
extern IvocVect* vector_new(int); // use this if possible
extern IvocVect* vector_new0();
extern IvocVect* vector_new1(int);
extern IvocVect* vector_new2(IvocVect*);
extern void vector_delete(IvocVect*);
extern int vector_buffer_size(IvocVect*);
extern int vector_capacity(IvocVect*);
extern void vector_resize(IvocVect*, int);
extern double* vector_vec(IvocVect*);
extern IvocVect* vector_arg(int);
extern bool is_vector_arg(int);
extern char* vector_get_label(IvocVect*);
extern void vector_set_label(IvocVect*, char*);
}

#endif

