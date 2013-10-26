#include <nrnconf.h>
#include "tqueue.h"
#include "pool.h"

#if COLLECT_TQueue_STATISTICS
#define STAT(arg) ++arg;
#else
#define STAT(arg) /**/
#endif

//----------------

implementPool(TQItemPool, TQItem)

#if BBTQ == 5
#include <sptbinq.cpp>
#else
#error "BBTQ must be 5"
#endif

SelfQueue::SelfQueue(TQItemPool* tp, int mkmut) {
	MUTCONSTRUCT(mkmut)
	tpool_ = tp;
	head_ = NULL;
}
SelfQueue::~SelfQueue() {
	remove_all();
	MUTDESTRUCT
}
TQItem* SelfQueue::insert(void* d) {
	MUTLOCK
	TQItem* q = tpool_->alloc();
	q->left_ = NULL;
	q->right_ = head_;
	if (head_) { head_->left_ = q; }
	head_ = q;
	q->data_ = d;
	MUTUNLOCK
	return q;
}
void* SelfQueue::remove(TQItem* q) {
	MUTLOCK
	if (q->left_) { q->left_->right_ = q->right_; }
	if (q->right_) { q->right_->left_ = q->left_; }
	if (q == head_) { head_ = q->right_; }
	tpool_->hpfree(q);
	MUTUNLOCK
	return q->data_;
}
void SelfQueue::remove_all() {
	MUTLOCK
	for (TQItem* q = first(); q; q = next(q)) {
		tpool_->hpfree(q);
	}
	head_ = NULL;
	MUTUNLOCK
}


