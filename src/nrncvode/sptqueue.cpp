// splay tree for event-sets or priority queues
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
}
	
void TQueue::print() {
#if FAST_LEAST
	if (least_) {
		prnt(least_, 0);
	}
#endif
	spscan(prnt, nil, sptree_);
}

void TQueue::forall_callback(void(*f)(const TQItem*, int)) {
#if FAST_LEAST
	if (least_) {
		f(least_, 0);
	}
#endif
	spscan(f, nil, sptree_);
}

void TQueue::check(const char* mes) {
}

#if !FAST_LEAST
double TQueue::least_t() {
	TQItem* b = least();
	if (b) {
		return b->t_;
	}else{
		return 1e15;
	}
}

TQItem* TQueue::least() {
	STAT(nleast)
	return sphead(sptree_);
}
#endif

#if FAST_LEAST
TQItem* TQueue::second_least(double t) {
	assert(least_);
	TQItem* b = sphead(sptree_);
	if (b && b->t_ == t) {
		return b;
	}
	return 0;
}
#endif

void TQueue::move_least(double tnew) {
	TQItem* b = least();
	if (b) {
#if FAST_LEAST
		b->t_ = tnew;
		TQItem* nl = sphead(sptree_);
		if (nl) {
			if (tnew > nl->t_) {
				least_ = spdeq(&sptree_->root);
				spenq(b, sptree_);
			}
		}
#else
		move(b, tnew);
#endif
	}
}

void TQueue::move(TQItem* i, double tnew) {
	STAT(nmove)
#if FAST_LEAST
	if (i == least_) {
		move_least(tnew);
	}else if (tnew < least_->t_) {
		spdelete(i, sptree_);
		i->t_ = tnew;
		spenq(least_, sptree_);
		least_ = i;
	}else
#endif
	{
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
#endif
}

TQItem* TQueue::insert(double t, void* d) {
	STAT(ninsert);
	TQItem* i = tpool_->alloc();
	i->data_ = d;
	i->t_ = t;
	i->cnt_ = 0;
#if FAST_LEAST
	if (t < least_t()) {
		if (least()) {
			spenq(least(), sptree_);
		}
		least_ = i;
	}else{
		spenq(i, sptree_);
	}
#else
	spenq(i, sptree_);
#endif
	return i;
}

void TQueue::remove(TQItem* q) {
	STAT(nrem);
	if (q) {
#if FAST_LEAST
		if (q == least_) {
			if (sptree_->root) {
				least_ = spdeq(&sptree_->root);
			}else{
				least_ = nil;
			}
		}else{
			spdelete(q, sptree_);
		}
#else
		spdelete(q, sptree_);
#endif
		tpool_->hpfree(q);
	}
}

TQItem* TQueue::find(double t) {
	STAT(nfind)
#if FAST_LEAST
	if (t == least_t()) {
		return least();
	}
#endif
	TQItem* q;
	q = splookup(t, sptree_);
	return(q);
}

#include <spaux.c>
#include <sptree.c>
#include <spdaveb.c>

