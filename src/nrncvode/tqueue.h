#ifndef tqueue_h
#define tqueue_h
#undef check

#include <nrnmutdec.h>
#include <pool.h>

class TQItem;
using TQItemPool = MutexPool<TQItem>;

// 0 use bbtqueue, 2 use sptqueue, 4 use spt2queue, 5 use sptbinqueue
#define BBTQ 5

#define FAST_LEAST 1

#if BBTQ == 0
#include <bbtqueue.h>
#elif BBTQ == 2
#define SplayTQueue TQueue
#define SplayTQItem TQItem
#include <sptqueue.h>
#elif BBTQ == 4
#undef FAST_LEAST
// required
#define FAST_LEAST   1
#define Splay2TQueue TQueue
#define Splay2TQItem TQItem
#include <spt2queue.h>
#elif BBTQ == 5
#undef FAST_LEAST
// required
#define FAST_LEAST     1
#define SplayTBinQueue TQueue
#define SplayTBinQItem TQItem
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
