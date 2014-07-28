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

//#ifndef tqueue_h
//#define tqueue_h

// bin queue for the fixed step method for NetCons and PreSyns. Splay tree
// for others.
// fifo for the NetCons and PreSyns with same delay. Splay tree for
// others (especially SelfEvents).
// note that most methods below assume a TQItem is in the splay tree
// For the bin part, only insert_fifo, and remove make sense,
// and forall_callback does the splay tree first and then the bin (so
// not in time order)
// The bin part assumes a fixed step method.

#include <assert.h>

#define COLLECT_TQueue_STATISTICS 1
struct SPTREE;
class TQItemPool;

class TQItem {
public:
	TQItem();
	virtual ~TQItem();
	bool check();
	void clear(){};
public:
	void* data_;
	double t_;
	TQItem* left_;
	TQItem* right_;
	TQItem* parent_;
	int cnt_; // reused: -1 means it is in the splay tree, >=0 gives bin
};

// helper class for the TQueue (SplayTBinQueue).
class BinQ {
public:
	BinQ();
	virtual ~BinQ();
	void enqueue(double tt, TQItem*);
	void shift(double tt) { assert(!bins_[qpt_]); tt_ = tt; if (++qpt_ >= nbin_) { qpt_ = 0; }}
	TQItem* top() { return bins_[qpt_]; }
	TQItem* dequeue();
	double tbin() { return tt_; }
	// for iteration
	TQItem* first();
	TQItem* next(TQItem*);
	void remove(TQItem*);
	void resize(int);
#if COLLECT_TQueue_STATISTICS
public:
	int nfenq, nfdeq, nfrem;
#endif
private:
	double tt_; // time at beginning of qpt_ interval
	int nbin_, qpt_;
	TQItem** bins_;
};

class TQueue {
public:
	TQueue(TQItemPool*, int mkmut = 0);
	virtual ~TQueue();

#define FAST_LEAST 1 // Cc tmp fix by Alex due to new include
#if FAST_LEAST
	TQItem* least() {return least_;}
    //TQItem* second_least(double t);
#if USE_PTHREAD
	double least_t(){double tt; MUTLOCK; if (least_) { tt = least_->t_;}else{tt = 1e15;} MUTUNLOCK; return tt;}
#else
	double least_t(){if (least_) { return least_->t_;}else{return 1e15;}}
#endif
#else
	TQItem* least(); // does not remove from TQueue
	double least_t();
#endif
	TQItem* atomic_dq(double til);
	TQItem* insert(double t, void* data);
	TQItem* enqueue_bin(double t, void* data);
	TQItem* dequeue_bin() { return binq_->dequeue(); }
	void shift_bin(double _t_) { ++nshift_; binq_->shift(_t_); }
	double tbin() { return binq_->tbin(); }
	TQItem* top() { return binq_->top(); }
	void release(TQItem*);
	TQItem* find(double t);
	void remove(TQItem*);
	void move(TQItem*, double tnew);
	void move_least(double tnew);
	void print();
	void check(const char* errmess);
	void statistics();
	void spike_stat(double*);
	void forall_callback(void (*)(const TQItem*, int));
	int nshift_;
	void deleteitem(TQItem*);
private:
	double least_t_nolock(){if (least_) { return least_->t_;}else{return 1e15;}}
	void move_least_nolock(double tnew);
	SPTREE* sptree_;
	BinQ* binq_;
	TQItem* least_;
	TQItemPool* tpool_;
	MUTDEC
#if COLLECT_TQueue_STATISTICS
	unsigned long ninsert, nrem, nleast, nbal, ncmplxrem;
	unsigned long ncompare, nleastsrch, nfind, nfindsrch, nmove, nfastmove;
#endif
};

//#endif
