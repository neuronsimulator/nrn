#pragma once
#include "oc_ansi.h"  // nrn_cacheline_calloc

// create and manage a vector of arrays as a memory pool of those arrays
// the idea is to allow the possibility of some extra cache efficiency

// the chain of ArrayPool
// is only for extra items in a pool_ and no other fields are used.
// The arraypool can be inceased in size by calling grow(ninc) which
// will chain a new ArrayPool of that size. Note that threads using
// distinct ArrayPool chain items will not have a problem with cache
// line sharing.
// When the arraypool needs to grow itself then it
// doubles in size every time a chain ArrayPool is added.
// maxget() tells the most number of arraypool items used at once.
// note that grow_(ninc) implicitly assumes all existing space is in use
// (i.e. put == get) and hence put ends up as put+ninc. On the other
// hand the user callable grow(ninc) assumes NO space is in use,
// so also put == get and put is set back to get.

template <typename T>
class ArrayPool {
  public:
    ArrayPool(long count, long d2);
    ~ArrayPool();
    T* alloc();
    void hpfree(T*);
    int maxget() {
        return maxget_;
    }
    int size() {
        return count_;
    }
    void free_all();
    T* pool() {
        return pool_;
    }
    long get() {
        return get_;
    }
    long put() {
        return put_;
    }
    long nget() {
        return nget_;
    }
    long ntget() {
        return ntget_;
    }
    long d2() {
        return d2_;
    }
    T* element(long i) {
        return pool_ + i * d2_;
    }
    T** items() {
        return items_;
    }
    void grow(long ninc);
    ArrayPool* chain() {
        return chain_;
    }
    long chain_size() {
        return pool_size_;
    }

  private:
    void grow_(long ninc);

  private:
    T** items_;
    T* pool_;
    long pool_size_;
    long count_;
    long get_;
    long put_;
    long nget_;
    long ntget_;
    long maxget_;
    long d2_;
    ArrayPool* chain_;
    ArrayPool* chainlast_;
};


template <typename T>
ArrayPool<T>::ArrayPool(long count, long d2) {
    count_ = count;
    d2_ = d2;
    pool_ = (T*) nrn_cacheline_calloc((void**) &pool_, count_ * d2_, sizeof(T));
    pool_size_ = count;
    items_ = new T*[count_];
    for (long i = 0; i < count_; ++i) {
        items_[i] = pool_ + i * d2_;
    }
    get_ = 0;
    put_ = 0;
    nget_ = 0;
    ntget_ = 0;
    maxget_ = 0;
    chain_ = 0;
    chainlast_ = this;
}

template <typename T>
void ArrayPool<T>::grow(long ninc) {
    grow_(ninc);
    put_ = get_;
}

template <typename T>
void ArrayPool<T>::grow_(long ninc) {
    assert(get_ == put_);
    ArrayPool* p = new ArrayPool(ninc, d2_);
    chainlast_->chain_ = p;
    chainlast_ = p;
    long newcnt = count_ + ninc;
    T** itms = new T*[newcnt];
    long i, j;
    put_ += ninc;
    for (i = 0; i < get_; ++i) {
        itms[i] = items_[i];
    }
    for (i = get_, j = 0; j < ninc; ++i, ++j) {
        itms[i] = p->items_[j];
    }
    for (i = put_, j = get_; j < count_; ++i, ++j) {
        itms[i] = items_[j];
    }
    delete[] items_;
    delete[] p->items_;
    p->items_ = 0;
    items_ = itms;
    count_ = newcnt;
}

template <typename T>
ArrayPool<T>::~ArrayPool() {
    if (chain_) {
        delete chain_;
    }
    free(pool_);
    if (items_) {
        delete[] items_;
    }
}

template <typename T>
T* ArrayPool<T>::alloc() {
    if (nget_ >= count_) {
        grow_(count_);
    }
    T* item = items_[get_];
    get_ = (get_ + 1) % count_;
    ++nget_;
    ++ntget_;
    if (nget_ > maxget_) {
        maxget_ = nget_;
    }
    return item;
}

template <typename T>
void ArrayPool<T>::hpfree(T* item) {
    assert(nget_ > 0);
    items_[put_] = item;
    put_ = (put_ + 1) % count_;
    --nget_;
}

template <typename T>
void ArrayPool<T>::free_all() {
    ArrayPool* pp;
    long i;
    nget_ = 0;
    get_ = 0;
    put_ = 0;
    for (pp = this; pp; pp = pp->chain_) {
        for (i = 0; i < pp->pool_size_; ++i) {
            items_[put_++] = pp->pool_ + i * d2_;
        }
    }
    assert(put_ == count_);
    put_ = 0;
}
