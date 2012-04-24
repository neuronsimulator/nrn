//#ifndef tqueue_h
//#define tqueue_h

#define COLLECT_TQueue_STATISTICS 1

class TQItem {
public:
	TQItem();
	virtual ~TQItem();
	bool check();
	int wleft() { return (left_?left_->w_:0); }
	int wright() { return (right_?right_->w_:0); }
	void t_iterate(void (*)(const TQItem*, int), int);
	bool unbalanced();
	void clear(){}
public:
	void* data_;
	double t_;
	TQItem* left_;
	TQItem* right_;
	TQItem* parent_;
	int w_;
};

class TQueue {
public:
	TQueue();
	virtual ~TQueue();
	
	TQItem* least(); // does not remove from TQueue
	double least_t();
	TQItem* insert(double t, void* data_);
	TQItem* find(double t);
	void remove(TQItem*);
	void move(TQItem*, double tnew);
	void move_least(double tnew);
	void print();
	void check(const char* errmess);
	void statistics();
	void insert1(double t, TQItem*);
	void remove1(TQItem*);
	void forall_callback(void (*)(const TQItem*, int));
private:
	void reverse(TQItem*);
	void new_least();
private:
	TQItem* least_;
	TQItem* root_;
#if COLLECT_TQueue_STATISTICS
private:
	unsigned long ninsert, nrem, nleast, nbal, ncmplxrem;
	unsigned long  ncompare, nleastsrch, nfind, nfindsrch, nmove, nfastmove;
#endif
};
//#endif
