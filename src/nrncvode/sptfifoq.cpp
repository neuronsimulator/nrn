// splay tree + fifofor event-sets or priority queues
// this starts from the sptqueue.cpp file and adds a fifo

/* Derived from David Brower's c translation of pascal code by
Douglas Jones.
*/
/* The original c code is included from this file but note that instead
of struct _spblk, we are really using TQItem
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define SPBLK TQItem
#define leftlink left_
#define rightlink right_
#define uplink parent_
#define cnt cnt_
#define key t_
#include <sptree.h>

TQItem::TQItem() {
	left_ = 0;
	right_ = 0;
	parent_ = 0;
}

TQItem::~TQItem() {
}

static void deleteitem(TQItem* i) { // only one, semantics changed
	assert(i);
	tpool_->hpfree(i);
}

bool TQItem::check() {
#if DOCHECK
#endif
	return true;
}

static void prnt(const TQItem* b, int level) {
	int i;
	for (i=0; i < level; ++i) {
		printf("    ");
	}
	printf("%g %c %d Q=%p D=%p\n", b->t_, b->data_?'x':'o', b->cnt_, b, b->data_);
}

static void chk(TQItem* b, int level) {
	if (!b->check()) {
		hoc_execerror("chk failed", errmess_);
	}
}

TQueue::TQueue() {
	if (!tpool_) {
		tpool_ = new TQItemPool(1000);
	}
	sptree_ = new SPTREE;
	spinit(sptree_);
	fifo_ = new FifoQ;
	least_ = 0;

#if COLLECT_TQueue_STATISTICS
	nmove = ninsert = nrem = nleast = nbal = ncmplxrem = 0;
	nfastmove = ncompare = nleastsrch = nfind = nfindsrch = 0;
#endif
}

TQueue::~TQueue() {
	SPBLK* q;
	while((q = spdeq(&sptree_->root)) != nil) {
		deleteitem(q);
	}
	delete sptree_;
	while((q = fifo_->dequeue()) != nil) {
		deleteitem(q);
	}
	delete fifo_;
}
	
void TQueue::print() {
#if FAST_LEAST
	if (least_) {
		prnt(least_, 0);
	}
#endif
	spscan(prnt, nil, sptree_);
	for (TQItem* q = fifo_->first(); q; q = fifo_->next(q)) {
		prnt(q, 0);
	}
}

void TQueue::forall_callback(void(*f)(const TQItem*, int)) {
#if FAST_LEAST
	if (least_) {
		f(least_, 0);
	}
#endif
	spscan(f, nil, sptree_);
	for (TQItem* q = fifo_->first(); q; q = fifo_->next(q)) {
		f(q, 0);
	}
}

void TQueue::check(const char* mes) {
}

void TQueue::move_least(double tnew) {
	TQItem* b = least();
	if (b) {
		b->t_ = tnew;
		TQItem* nl = sphead(sptree_);
		if (nl) {
			if (tnew <= nl->t_ && tnew <= fifo_->least_t()) {
				;
			}else if (nl->t_ <= fifo_->least_t()) {
				least_ = spdeq(&sptree_->root);
				spenq(b, sptree_);
			}else{
				least_ = fifo_->dequeue();
				spenq(b, sptree_);
			}
		}else if (tnew > fifo_->least_t()) {
			least_ = fifo_->dequeue();
			spenq(b, sptree_);
		}
	}
}

void TQueue::move(TQItem* i, double tnew) {
	STAT(nmove)
	if (i == least_) {
		move_least(tnew);
	}else if (tnew < least_->t_) {
		spdelete(i, sptree_);
		i->t_ = tnew;
		spenq(least_, sptree_);
		least_ = i;
	}else{
		spdelete(i, sptree_);
		i->t_ = tnew;
		spenq(i, sptree_);
	}
}

void TQueue::statistics() {
#if COLLECT_TQueue_STATISTICS
	printf("insertions=%lu  moves=%lu removals=%lu calls to least=%lu\n",
		ninsert, nmove, nrem, nleast);
	printf("calls to find=%lu\n",
		nfind);
	printf("comparisons=%lu\n",
		sptree_->enqcmps);
#else
	printf("Turn on COLLECT_TQueue_STATISTICS_ in tqueue.h\n");
#endif
}

void TQueue::spike_stat(double* d) {
#if COLLECT_TQueue_STATISTICS
	d[0] = ninsert;
	d[1] = nmove;
	d[2] = nrem;
//printf("FifoQ spikestat nfenq=%lu nfdeq=%lu nfrem=%lu\n", fifo_->nfenq, fifo_->nfdeq, fifo_->nfrem);
#endif
}

TQItem* TQueue::insert(double t, void* d) {
	STAT(ninsert);
	TQItem* i = tpool_->alloc();
	i->data_ = d;
	i->t_ = t;
	i->cnt_ = 0;
	if (t < least_t()) {
		if (least()) {
			spenq(least(), sptree_);
		}
		least_ = i;
	}else{
		spenq(i, sptree_);
	}
	return i;
}

TQItem* TQueue::insert_fifo(double t, void* d) {
	STAT(ninsert);
	TQItem* i = tpool_->alloc();
	i->data_ = d;
	i->t_ = t;
	i->cnt_ = 0;
	if (t < least_t()) {
		if (least()) {
			spenq(least(), sptree_);
		}
		least_ = i;
	}else{
		fifo_->enqueue(i);
	}
	return i;
}

void TQueue::remove(TQItem* q) {
	STAT(nrem);
	if (q) {
		if (q == least_) {
			if (sptree_->root) {
				TQItem* nl = sphead(sptree_);
				if (nl->t_ <= fifo_->least_t()) {
					least_ = spdeq(&sptree_->root);
				}else{
					least_ = fifo_->dequeue();
				}
			}else{
				least_ = fifo_->dequeue();
			}
		}else{
			if (q->cnt_ == -1) {
				fifo_->remove(q);
			}else{
				spdelete(q, sptree_);
			}
		}
		tpool_->hpfree(q);
	}
}

TQItem* TQueue::find(double t) {
	// search only in the  splay tree. if this is a bug then fix it.
	STAT(nfind)
	if (t == least_t()) {
		return least();
	}
	TQItem* q;
	q = splookup(t, sptree_);
	return(q);
}

FifoQ::FifoQ() {
	head_ = tail_ = 0;
#if COLLECT_TQueue_STATISTICS
	nfenq = nfdeq = nfrem = 0;
#endif
}

FifoQ::~FifoQ() {
	assert (head_ == 0 && tail_ == 0);
}

double FifoQ::least_t() {
	if (head_) { return head_->t_; }
	return 1e50; // must be larger than any possible t
}
void FifoQ::enqueue(TQItem* q) {
	if (tail_) { assert(tail_->t_ <= q->t_); }
	q->cnt_ = -1;
	q->left_ = tail_;
	q->right_ = 0;
	tail_ = q;
	if (head_) { tail_->left_->right_ = q;} else { head_ = tail_; }
#if COLLECT_TQueue_STATISTICS
	++nfenq;
#endif
}
TQItem* FifoQ::dequeue() {
	TQItem* q = head_;
	if (head_ == tail_) {
		head_ = tail_ = 0;
	}else{
		head_ = head_->right_;
		head_->left_ = 0;
	}
	if (q) {
		q->left_ = q->right_ = 0;
		q->cnt_ = 0;
	}
#if COLLECT_TQueue_STATISTICS
	++nfdeq;
#endif
	return q;
}
void FifoQ::remove(TQItem* q) {
	printf("FifoQ remove %p\n", q);
	if (head_ == q) { head_ = q->right_; }
	if (tail_ == q) { tail_ = q->left_; }
	if (q->left_) { q->left_->right_ = q->right_; }
	if (q->right_) { q->right_->left_ = q->left_; }
	q->left_ = q->right_ = 0;
	q->cnt_ = 0;
#if COLLECT_TQueue_STATISTICS
	++nfrem;
#endif
}

TQItem* FifoQ::first() {
	return head_;
}
TQItem* FifoQ::next(TQItem* q) {
	return q->right_;
}

#include <spaux.c>
#include <sptree.c>
#include <spdaveb.c>

