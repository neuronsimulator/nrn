//#ifndef tqueue_h
//#define tqueue_h

// fifo for the NetCons and PreSyns with same delay. Splay tree for
// others (especially SelfEvents).
// note that most methods below assume a TQItem is in the splay tree
// For the fifo part, only insert_fifo, and remove make sense,
// and forall_callback does the splay tree first and then the fifo (so
// not in time order)
#define COLLECT_TQueue_STATISTICS 1
struct SPTREE;

class TQItem {
public:
	TQItem();
	virtual ~TQItem();
	bool check();
	void clear(){};
public:
	void* data_;
	double t_;
	TQItem* left_; // in the fifo, this is toward the head
	TQItem* right_; // in the fifo, this is toward the tail
	TQItem* parent_; // in the fifo, this is unused
	int cnt_; // reused: -1 means it is in the fifo
};

// helper class for the TQueue (SplayTFifoQueue).
class FifoQ {
public:
	FifoQ();
	virtual ~FifoQ();
	double least_t();
	void enqueue(TQItem*);
	TQItem* dequeue();
	void remove(TQItem*);
	// for iteration
	TQItem* first();
	TQItem* next(TQItem*);
#if COLLECT_TQueue_STATISTICS
public:
	int nfenq, nfdeq, nfrem;
#endif
private:
	TQItem* head_;
	TQItem* tail_;
};

class TQueue {
public:
	TQueue();
	virtual ~TQueue();
	
#if FAST_LEAST
	TQItem* least() {return least_;}
	double least_t(){if (least_) { return least_->t_;}else{return 1e15;}}
#else
	TQItem* least(); // does not remove from TQueue
	double least_t();
#endif
	TQItem* insert(double t, void* data_);
	TQItem* insert_fifo(double t, void* data_);
	TQItem* find(double t);
	void remove(TQItem*);
	void move(TQItem*, double tnew);
	void move_least(double tnew);
	void print();
	void check(const char* errmess);
	void statistics();
	void spike_stat(double*);
	void forall_callback(void (*)(const TQItem*, int));
private:
	SPTREE* sptree_;
	FifoQ* fifo_;
	TQItem* least_;
#if COLLECT_TQueue_STATISTICS
	unsigned long ninsert, nrem, nleast, nbal, ncmplxrem;
	unsigned long ncompare, nleastsrch, nfind, nfindsrch, nmove, nfastmove;
#endif
};
//#endif
