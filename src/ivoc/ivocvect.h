#ifndef ivoc_vector_h
#define ivoc_vector_h

// definition of vector classes from the gnu c++ class library
#include <nrnmutdec.h>
#include <vector>
#include <numeric>
#include <algorithm>

extern "C" {
extern void notify_freed_val_array(double *, size_t);
}

#define Vect IvocVect
using ParentVect = std::vector<double>;
struct Object;

class IvocVect {
public:
	IvocVect(Object* obj = NULL);
	IvocVect(int, Object* obj = NULL);
	IvocVect(int, double, Object* obj = NULL);
	IvocVect(IvocVect&, Object* obj = NULL);
	~IvocVect();

	Object** temp_objvar();
	int buffer_size();
	void buffer_size(int);
	void label(const char*);

    inline double& elem(int n) {
        return vec_.at(n);
    }

    inline std::vector<double>& vec() {
        return vec_;
    }

    inline double* data() {
        return vec_.data();
    }

    inline size_t size() const {
        return vec_.size();
    }

    inline void resize(size_t n) {
        if(n > vec_.size()) {
            notify_freed_val_array(vec_.data(), vec_.size());
        }
        vec_.resize(n);
    }

    inline void resize(size_t n, double fill_value) {
        if(n > vec_.size()) {
            notify_freed_val_array(vec_.data(), vec_.size());
        }
        vec_.resize(n, fill_value);
    }

    inline double& operator[] (size_t index) {
        return vec_.at(index);
    }

    inline auto begin() -> std::vector<double>::iterator {
        return vec_.begin();
    }

    inline auto end() -> std::vector<double>::iterator {
        return vec_.end();
    }

    inline void push_back(double v) {
        vec_.push_back(v);
    }

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
	std::vector<double> vec_;   // std::vector holding data
	MUTDEC
};

template< class InputIterator >
double var(InputIterator begin, InputIterator end) {
    const size_t size = end-begin;
    const double sum = std::accumulate(begin, end, 0.0);
    const double m =  sum / size;

    double accum = 0.0;
    std::for_each (begin, end, [&](const double d) {
        accum += (d - m) * (d - m);
    });

    return accum / (size-1);

}

template< class InputIterator >
double stdDev(InputIterator begin, InputIterator end) {
    return sqrt(var(begin, end));
}

extern "C" {
extern Vect* vector_new(int, Object*); // use this if possible
extern Vect* vector_new0();
extern Vect* vector_new1(int);
extern Vect* vector_new2(Vect*);
extern void vector_delete(Vect*);
extern int vector_buffer_size(Vect*);
extern Object** vector_temp_objvar(Vect*);
extern Object** vector_pobj(Vect*);

extern int is_vector_arg(int);
extern char* vector_get_label(Vect*);
extern void vector_set_label(Vect*, char*);
} // extern "C"

#if defined(__cplusplus)
extern "C" {
#endif

extern Vect* vector_arg(int);
extern double* vector_vec(Vect*);
extern int vector_capacity(Vect*);
extern void vector_resize(Vect*, int);

#if defined(__cplusplus)
}
#endif

#endif

