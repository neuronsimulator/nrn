//#ifndef tqueue_h
//#define tqueue_h

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
	int cnt_;
};

class TQueue {
public:
	TQueue();
	virtual ~TQueue();
	
#if FAST_LEAST
	TQItem* least() {return least_;}
	double least_t(){if (least_) { return least_->t_;}else{return 1e15;}}
	TQItem* second_least(double t);
#else
	TQItem* least(); // does not remove from TQueue
	double least_t();
#endif
	TQItem* insert(double t, void* data_);
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
	TQItem* least_;
#if COLLECT_TQueue_STATISTICS
	unsigned long ninsert, nrem, nleast, nbal, ncmplxrem;
	unsigned long ncompare, nleastsrch, nfind, nfindsrch, nmove, nfastmove;
#endif
};
//#endif
