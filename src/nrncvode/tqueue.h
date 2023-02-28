#ifndef tqueue_h
#define tqueue_h
#undef check

#include <nrnmutdec.h>
#include <pool.h>

class TQItem;
using TQItemPool = MutexPool<TQItem>;

// 0 use bbtqueue, 2 use sptqueue, 4 use spt2queue, 5 use sptbinqueue
#define BBTQ 5

#if BBTQ == 0
#include <bbtqueue.h>
#elif BBTQ == 2
#include <sptqueue.h>
#elif BBTQ == 4
#include <spt2queue.h>
#elif BBTQ == 5
#include <sptbinq.h>
#endif

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
