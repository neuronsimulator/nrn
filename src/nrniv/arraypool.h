#ifndef arraypool_h
#define arraypool_h

// create and manage a vector of arrays as a memory pool of those arrays
// the idea is to allow the possibility of some extra cache efficiency

// the chain of ArrayPool
// is only for extra items in a pool_ and no other fields are used.
// the arraypool doubles in size every time a chain ArrayPool is added.
// maxget() tells the most number of arraypool items used at once.

#define declareArrayPool(ArrayPool,T) \
class ArrayPool { \
public: \
	ArrayPool(long count, long d2); \
	~ArrayPool(); \
	T* alloc(); \
	void hpfree(T*); \
	int maxget() { return maxget_;} \
	int size() { return count_;} \
	void free_all(); \
	T* pool() { return pool_; } \
	long get() { return get_; } \
	long put() { return put_; } \
	long nget() { return nget_; } \
	long d2() { return d2_; } \
	T* element(long i) { return pool_ + i*d2_; } \
private: \
	void grow(); \
private: \
	T** items_; \
	T* pool_; \
	long pool_size_; \
	long count_; \
	long get_; \
	long put_; \
	long nget_; \
	long maxget_; \
	long d2_; \
	ArrayPool* chain_; \
}; \
 \

#define implementArrayPool(ArrayPool,T) \
ArrayPool::ArrayPool(long count, long d2) { \
	count_ = count; \
	d2_ = d2; \
	pool_ = (T*)hoc_Ecalloc(count_*d2_, sizeof(T)); hoc_malchk(); \
	pool_size_ = count; \
	items_ = new T*[count_]; \
	for (long i = 0; i < count_; ++i) items_[i] = pool_ + i*d2_; \
	get_ = 0; \
	put_ = 0; \
	nget_ = 0; \
	maxget_ = 0; \
	chain_ = 0; \
} \
 \
void ArrayPool::grow() { \
	assert(get_ == put_); \
	ArrayPool* p = new ArrayPool(count_, d2_); \
	p->chain_ = chain_; \
	chain_ = p; \
	long newcnt = 2*count_; \
	T** itms = new T*[newcnt]; \
	long i, j; \
	put_ += count_; \
	for (i = 0; i < get_; ++i) { \
		itms[i] = items_[i]; \
	} \
	for (i = get_, j = 0; j < count_; ++i, ++j) { \
		itms[i] = p->items_[j]; \
	} \
	for (i = put_, j = get_; j < count_; ++i, ++j) { \
		itms[i] = items_[j]; \
	} \
	delete [] items_; \
	delete [] p->items_; \
	p->items_ = 0; \
	items_ = itms; \
	count_ = newcnt; \
} \
 \
ArrayPool::~ArrayPool() { \
	if (chain_) { \
		delete chain_; \
	} \
	free(pool_); \
	if (items_) { \
		delete [] items_; \
	} \
} \
 \
T* ArrayPool::alloc() { \
	if (nget_ >= count_) { grow(); } \
	T* item = items_[get_]; \
	get_ = (get_+1)%count_; \
	++nget_; \
	if (nget_ > maxget_) { maxget_ = nget_; } \
	return item; \
} \
 \
void ArrayPool::hpfree(T* item) { \
	assert(nget_ > 0); \
	items_[put_] = item; \
	put_ = (put_ + 1)%count_; \
	--nget_; \
} \
\
void ArrayPool::free_all() { \
	ArrayPool* pp; \
	long i; \
	nget_ = 0; \
	get_ = 0; \
	put_ = 0; \
	for(pp = this; pp; pp = pp->chain_) { \
		for (i=0; i < pp->pool_size_; ++i) { \
			items_[put_++] = pp->pool_ + i*d2_; \
		} \
	} \
	assert(put_ == count_); \
	put_ = 0; \
} \
\

#endif
