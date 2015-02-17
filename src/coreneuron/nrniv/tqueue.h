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

#ifndef tqueue_h
#define tqueue_h

#include "coreneuron/nrniv/nrnmutdec.h"
#include "coreneuron/nrniv/pool.h"
#include "coreneuron/nrniv/sptbinq.h"

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
