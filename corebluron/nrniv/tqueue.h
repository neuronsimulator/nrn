#ifndef tqueue_h
#define tqueue_h

#include "corebluron/nrniv/nrnmutdec.h"
#include "corebluron/nrniv/pool.h"
#include "corebluron/nrniv/sptbinq.h"

#undef check

class TQItem;
declarePool(TQItemPool, TQItem)

// 0 use bbtqueue, 1 use rbtqueue, 2 use sptqueue, 3 use sptfifoq
#define BBTQ 5
#define FAST_LEAST 1
#define SplayTBinQueue TQueue
#define SplayTBinQItem TQItem

class SelfQueue { // not really a queue but a doubly linked list for fast
public:		  // insertion, deletion, iteration
	SelfQueue(TQItemPool*, int mkmut = 0);
	virtual ~SelfQueue();
	TQItem* insert(void*);
	void* remove(TQItem*);
	void remove_all();
	TQItem* first() { return head_; }
	TQItem* next(TQItem* q) { return q->right_; }
private:
	TQItem* head_;
	TQItemPool* tpool_;
	MUTDEC
};

#endif
