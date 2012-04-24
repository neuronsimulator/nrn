//#ifndef tqueue_h
//#define tqueue_h

// second splay tree for the NetCons and PreSyns with same delay. First
// splay tree for others (especially SelfEvents).
// forall_callback does one splay tree first and then the other (so
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
	TQItem* left_;
	TQItem* right_;
	TQItem* parent_;
	int cnt_; // reused: -1 means it is in the second splay tree
};

class TQueue {
public:
	TQueue();
	virtual ~TQueue();
	
#if FAST_LEAST // must be on
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
	double q2least_t();
private:
	SPTREE* sptree_;
	SPTREE* sptree2_;
	TQItem* least_;
#if COLLECT_TQueue_STATISTICS
	unsigned long ninsert, nrem, nleast, nbal, ncmplxrem;
	unsigned long ncompare, nleastsrch, nfind, nfindsrch, nmove, nfastmove;
	unsigned long nenq1, nenq2;
#endif
};
//#endif
