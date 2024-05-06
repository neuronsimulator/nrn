#pragma once

// same as ../nrncvode/pool.h but items do not require a clear method.

// create and manage a vector of objects as a memory pool of those objects
// the object must have a void clear() method which takes care of any
// data the object contains which should be deleted upon free_all.
// clear() is NOT called on free, only on free_all.

// the chain of Pool
// is only for extra items in a pool_ and no other fields are used.
// the pool doubles in size every time a chain Pool is added.
// maxget() tells the most number of pool items used at once.

template <typename T>
class Pool {
  public:
    Pool(long count);
    ~Pool();
    T* alloc();
    void hpfree(T*);
    int maxget() {
        return maxget_;
    }
    void free_all();
    int is_valid_ptr(void*);

  private:
    void grow();

  private:
    T** items_;
    T* pool_;
    long pool_size_;
    long count_;
    long get_;
    long put_;
    long nget_;
    long maxget_;
    Pool* chain_;
};


template <typename T>
Pool<T>::Pool(long count) {
    count_ = count;
    pool_ = new T[count_];
    pool_size_ = count;
    items_ = new T*[count_];
    for (long i = 0; i < count_; ++i)
        items_[i] = pool_ + i;
    get_ = 0;
    put_ = 0;
    nget_ = 0;
    maxget_ = 0;
    chain_ = 0;
}

template <typename T>
void Pool<T>::grow() {
    assert(get_ == put_);
    Pool* p = new Pool(count_);
    p->chain_ = chain_;
    chain_ = p;
    long newcnt = 2 * count_;
    T** itms = new T*[newcnt];
    long i, j;
    put_ += count_;
    for (i = 0; i < get_; ++i) {
        itms[i] = items_[i];
    }
    for (i = get_, j = 0; j < count_; ++i, ++j) {
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
Pool<T>::~Pool() {
    if (chain_) {
        delete chain_;
    }
    delete[] pool_;
    if (items_) {
        delete[] items_;
    }
}

template <typename T>
int Pool<T>::is_valid_ptr(void* v) {
    Pool* pp;
    for (pp = this; pp; pp = pp->chain_) {
        void* vp = (void*) (pp->pool_);
        if (v >= vp && v < (void*) (pp->pool_ + pp->pool_size_)) {
            if ((((char*) v - (char*) vp) % sizeof(T)) == 0) {
                return 1;
            } else {
                return 0;
            }
        }
    }
    return 0;
}

template <typename T>
T* Pool<T>::alloc() {
    if (nget_ >= count_) {
        grow();
    }
    T* item = items_[get_];
    get_ = (get_ + 1) % count_;
    ++nget_;
    if (nget_ > maxget_) {
        maxget_ = nget_;
    }
    return item;
}

template <typename T>
void Pool<T>::hpfree(T* item) {
    assert(nget_ > 0);
    items_[put_] = item;
    put_ = (put_ + 1) % count_;
    --nget_;
}

template <typename T>
void Pool<T>::free_all() {
    Pool* pp;
    long i;
    nget_ = 0;
    get_ = 0;
    put_ = 0;
    for (pp = this; pp; pp = pp->chain_) {
        for (i = 0; i < pp->pool_size_; ++i) {
            items_[put_++] = pp->pool_ + i;
        }
    }
    assert(put_ == count_);
    put_ = 0;
}
