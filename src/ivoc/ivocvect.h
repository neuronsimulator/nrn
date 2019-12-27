#ifndef ivoc_vector_h
#define ivoc_vector_h

// definition of vector classes from the gnu c++ class library
#include <d_avec.h>  
#include <nrnmutdec.h>

#define ParentVect doubleAVec
#define Vect IvocVect

struct Object;

class IvocVect : public ParentVect {
public:
	IvocVect(Object* obj = NULL);
	IvocVect(int, Object* obj = NULL);
	IvocVect(int, double, Object* obj = NULL);
	IvocVect(IvocVect&, Object* obj = NULL);
	~IvocVect();

	void resize(int);
	void resize_chunk(int len, int realloc_extra = 0);
	// volatile, only one at a time, don't do anything
	// that changes the memory space.
	IvocVect* subvec(int start, int end);

	Object** temp_objvar();
	int buffer_size();
	void buffer_size(int);
	void label(const char*);

#if USE_PTHREAD
	void mutconstruct(int mkmut) {if (!mut_) MUTCONSTRUCT(mkmut)}
#else
	void mutconstruct(int) {}
#endif
	void lock() {MUTLOCK}
	void unlock() {MUTUNLOCK}
public:
	//intended as friend static Object** temp_objvar(IvocVect*);
	Object* obj_;	// so far only needed by record and play; not reffed
	char* label_;
	MUTDEC
};


extern "C" {
extern Vect* vector_new(int, Object*); // use this if possible
extern Vect* vector_new0();
extern Vect* vector_new1(int);
extern Vect* vector_new2(Vect*);
extern void vector_delete(Vect*);
extern int vector_buffer_size(Vect*);
extern int vector_capacity(Vect*);
extern void vector_resize(Vect*, int);
extern Object** vector_temp_objvar(Vect*);
extern double* vector_vec(Vect*);
extern Object** vector_pobj(Vect*);
extern Vect* vector_arg(int);
extern int is_vector_arg(int);
extern char* vector_get_label(Vect*);
extern void vector_set_label(Vect*, char*);
}

#endif

