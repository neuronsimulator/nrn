#ifndef tqueue_h
#define tqueue_h
#undef check

#include <nrnmutdec.h>
#include <pool.h>

class TQItem;
using TQItemPool = MutexPool<TQItem>;

#include <sptbinq.h>

class SelfQueue {  // not really a queue but a doubly linked list for fast
  public:          // insertion, deletion, iteration
    SelfQueue(TQItemPool*, int mkmut = 0);
    virtual ~SelfQueue();
    TQItem* insert(void*);
    void* remove(TQItem*);
    void remove_all();
    TQItem* first() {
        return head_;
    }
    TQItem* next(TQItem* q) {
        return q->right_;
    }

  private:
    TQItem* head_;
    TQItemPool* tpool_;
    MUTDEC
};

#endif
