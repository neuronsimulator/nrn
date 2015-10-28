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

/*
**  SPTREE:  The following type declarations provide the binary tree
**  representation of event-sets or priority queues needed by splay trees
**
**  assumes that data and datb will be provided by the application
**  to hold all application specific information
**
**  assumes that key will be provided by the application, comparable
**  with the compare function applied to the addresses of two keys.
*/
// bin queue for the fixed step method for NetCons and PreSyns. Splay tree
// for others.
// fifo for the NetCons and PreSyns with same delay. Splay tree for
// others (especially SelfEvents).
// note that most methods below assume a TQItem is in the splay tree
// For the bin part, only insert_fifo, and remove make sense,
// The bin part assumes a fixed step method.

#include <stdio.h>
#include <assert.h>
#include "coreneuron/nrniv/nrnmutdec.h"

#define COLLECT_TQueue_STATISTICS 1

#define STRCMP(a, b) (a - b)

class TQItem;
#define SPBLK TQItem
#define leftlink left_
#define rightlink right_
#define uplink parent_
#define cnt cnt_
#define key t_

typedef struct SPTREE
{
    SPBLK	* root;		/* root node */

    /* Statistics, not strictly necessary, but handy for tuning  */
    int		enqcmps;	/* compares in spenq */

} SPTREE;

#define spinit sptq_spinit
#define spenq sptq_spenq
#define spdeq sptq_spdeq
#define splay sptq_splay
#define sphead sptq_sphead
#define spdelete sptq_spdelete

extern void spinit(SPTREE*);		/* init tree */
extern SPBLK * spenq(SPBLK*, SPTREE*);	/* insert item into the tree */
extern SPBLK * spdeq(SPBLK**);		/* return and remove lowest item in subtree */
extern void splay(SPBLK*, SPTREE*);	/* reorganize tree */
extern SPBLK * sphead(SPTREE*);         /* return first node in tree */
extern void spdelete(SPBLK*, SPTREE*);	/* delete node from tree */


class TQItem {
public:
	TQItem();
	virtual ~TQItem();
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
	// for iteration
	TQItem* first();
	TQItem* next(TQItem*);
	void remove(TQItem*);
    void resize(int);
#if COLLECT_TQueue_STATISTICS
public:
    int nfenq;
#endif
private:
	double tt_; // time at beginning of qpt_ interval
	int nbin_, qpt_;
	TQItem** bins_;
};

class TQueue {
public:
  TQueue();
  virtual ~TQueue();

  TQItem* least() {return least_;}
  TQItem* atomic_dq(double til);
  TQItem* insert(double t, void* data);
  void shift_bin(double _t_) { ++nshift_; binq_->shift(_t_); }
  void remove(TQItem*);
  void move(TQItem*, double tnew);
  void statistics();
  int nshift_;

private:
  double least_t_nolock(){if (least_) { return least_->t_;}else{return 1e15;}}
  void move_least_nolock(double tnew);
  SPTREE* sptree_;
  BinQ* binq_;
  TQItem* least_;
  MUTDEC
#if COLLECT_TQueue_STATISTICS
  unsigned long ninsert, nrem, nleast, nbal, ncmplxrem;
  unsigned long ncompare, nleastsrch, nfind, nfindsrch, nmove, nfastmove;
#endif
};


//#endif
