/*
Copyright (c) 2014 EPFL-BBP, All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE BLUE BRAIN PROJECT "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE BLUE BRAIN PROJECT
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "corebluron/nrnoc/multicore.h"
#include "corebluron/nrniv/tqueue.h"
#include "corebluron/nrniv/pool.h"

#if COLLECT_TQueue_STATISTICS
#define STAT(arg) ++arg;
#else
#define STAT(arg) /**/
#endif

//----------------

implementPool(TQItemPool, TQItem)

#include "corebluron/nrniv/sptbinq.cpp"

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


