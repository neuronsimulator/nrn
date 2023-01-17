#ifndef tqueue_h
#define tqueue_h
#undef check

#include <nrnmutdec.h>
#include <pool.h>

class TQItem;
declarePool(TQItemPool, TQItem)

// 0 use bbtqueue, 1 use rbtqueue, 2 use sptqueue, 3 use sptfifoq
#define BBTQ 5

#define FAST_LEAST 1

#if BBTQ == 0
#include <bbtqueue.h>
#endif

#if BBTQ == 1
#define RBTQueue TQueue
#define RBTQItem TQItem
#include <rbtqueue.h>
#endif

#if BBTQ == 2
#define SplayTQueue TQueue
#define SplayTQItem TQItem
#include <sptqueue.h>
#endif

#if BBTQ == 3
// note: cannot use the fifo queue for parallel simulations since
// the received MPI_Allgather buffer is not properly time sorted.
#undef FAST_LEAST
// required
#define FAST_LEAST      1
#define SplayTFifoQueue TQueue
#define SplayTFifoQItem TQItem
#include <sptfifoq.h>
#endif

#if BBTQ == 4
#undef FAST_LEAST
// required
#define FAST_LEAST   1
#define Splay2TQueue TQueue
#define Splay2TQItem TQItem
#include <spt2queue.h>
#endif

#if BBTQ == 5
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
