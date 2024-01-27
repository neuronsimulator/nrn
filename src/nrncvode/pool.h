#ifndef pool_h
#define pool_h

// create and manage a vector of objects as a memory pool of those objects
// the object must have a void clear() method which takes care of any
// data the object contains which should be deleted upon free_all.
// clear() is NOT called on free, only on free_all.

// the chain of Pool
// is only for extra items in a pool_ and no other fields are used.
// the pool doubles in size every time a chain Pool is added.
// maxget() tells the most number of pool items used at once.

#include <nrnmutdec.h>

#include <cassert>

template <typename T>
class MutexPool {
  public:
    MutexPool(long count, int mkmut = 0);
    ~MutexPool();
    T* alloc();
    void hpfree(T*);
    int maxget() {
        return maxget_;
    }
    void free_all();

  private:
    void grow();
    T** items_{};
    T* pool_{};
    long pool_size_{};
    long count_{};
    long get_{};
    long put_{};
    long nget_{};
    long maxget_{};
    MutexPool<T>* chain_{};
    MUTDEC
};

template <typename T>
MutexPool<T>::MutexPool(long count, int mkmut) {
    count_ = count;
    pool_ = new T[count_];
    pool_size_ = count;
    items_ = new T*[count_];
    for (long i = 0; i < count_; ++i) {
        items_[i] = pool_ + i;
    }
    MUTCONSTRUCT(mkmut)
}

template <typename T>
void MutexPool<T>::grow() {
    assert(get_ == put_);
    MutexPool<T>* p = new MutexPool<T>(count_);
    p->chain_ = chain_;
    chain_ = p;
    long newcnt = 2 * count_;
    T** itms = new T*[newcnt];
    put_ += count_;
    for (long i = 0; i < get_; ++i) {
        itms[i] = items_[i];
    }
    for (long i = get_, j = 0; j < count_; ++i, ++j) {
        itms[i] = p->items_[j];
    }
    for (long i = put_, j = get_; j < count_; ++i, ++j) {
        itms[i] = items_[j];
    }
    delete[] items_;
    delete[] p->items_;
    p->items_ = 0;
    items_ = itms;
    count_ = newcnt;
}

template <typename T>
MutexPool<T>::~MutexPool() {
    delete chain_;
    delete[] pool_;
    delete[] items_;
    MUTDESTRUCT
}

template <typename T>
T* MutexPool<T>::alloc() {
    MUTLOCK
    if (nget_ >= count_) {
        grow();
    }
    T* item = items_[get_];
    get_ = (get_ + 1) % count_;
    ++nget_;

    maxget_ = std::max(nget_, maxget_);

    MUTUNLOCK
    return item;
}

template <typename T>
void MutexPool<T>::hpfree(T* item) {
    MUTLOCK
    assert(nget_ > 0);
    items_[put_] = item;
    put_ = (put_ + 1) % count_;
    --nget_;
    MUTUNLOCK
}

template <typename T>
void MutexPool<T>::free_all() {
    MUTLOCK
    MutexPool<T>* pp;
    long i;
    nget_ = 0;
    get_ = 0;
    put_ = 0;
    for (pp = this; pp; pp = pp->chain_) {
        for (i = 0; i < pp->pool_size_; ++i) {
            items_[put_++] = pp->pool_ + i;
            pp->pool_[i].clear();
        }
    }
    assert(put_ == count_);
    put_ = 0;
    MUTUNLOCK
}


#endif
